const ss = use("rtmpUrls");

var http = require("http"),
  https = require("https"),
  sio = require("socket.io"),
  fs = require("fs"),
  Peer = require("./peer"),
  adapter = require("./adapter"),
  debug = require("debug")("symple:server");

const mongodb = use("mongodb")();
var crypto = require("crypto");

let Clients = {};
let Rooms = {};
let Critical = {};

function getTURNCredentials(name, secret) {
  var unixTimeStamp = parseInt(Date.now() / 1000) + 24 * 3600, // this credential would be valid for the next 24 hours
    username = String(unixTimeStamp + name),
    password,
    hmac = crypto.createHmac("sha1", secret);
  hmac.setEncoding("base64");
  hmac.write(username);
  hmac.end();
  password = hmac.read();
  return {
    username: username,
    password: password
  };
}

async function makeIceArray() {
  const ice_servers = await (await mongodb).IceServers.get();
  const result = new Array(ice_servers.length);
  ice_servers.forEach((ice_server, idx) => {
    result[idx] = {
      host: ice_server.host,
      port: ice_server.port,
      type: ice_server.type,
      owner: ice_server.owner
    };
    if (ice_server.auth) {
      result[idx].credentials = getTURNCredentials(
        new Date().getTime(),
        ice_server.secret || "mysecret"
      );
    }
  });
  return result;
}

async function pickNodeToConnect(clientWantedConnect, room) {
  let coll = (await mongodb).collection("clients" + room);

  let cl = (await coll.findOneAndUpdate(
    {
      nconnects: { $lt: 4 },
      connects: { $nin: [clientWantedConnect.sock_id] },
      sock_id: { $ne: clientWantedConnect.sock_id }
    },
    {
      $inc: { nconnects: 1 },
      $push: { connects: clientWantedConnect.sock_id }
    }
  )).value;

  return cl;
}
/**
 * @class
 * Symple server class.
 *
 * @param {Object} config - optional confguration object (see config.json)
 * @param {string} config.port - the port to listen on
 * @param {Object} config.redis - redis configuration
 * @param {Object} config.ssl - ssl configuration
 * @param {number} config.sessionTTL - session timeout value
 * @param {boolean} config.authentication - enable/disable authentication
 * @public
 */

function Symple(config) {
  this.config = config || {};
  this.timer = undefined;
}

/**
 * Load configuration from a file.
 *
 * @param {string} filepath - absolute path to config.json
 * @public
 */

Symple.prototype.loadConfig = function(filepath) {
  this.config = JSON.parse(
    fs
      .readFileSync(filepath)
      .toString()
      .replace(
        //
        new RegExp("\\/\\*(.|\\r|\\n)*?\\*\\/", "g"),
        "" // strip out comments
      )
  );
};
/**
 * Initialize the Symple server.
 *
 * @public
 */

Symple.prototype.init = function() {
  this.initHTTP();
  this.initSocketIO();
  this.initRedis();
  let self = this;
  // const sync = async () => {
  //   let rooms = Object.keys(Rooms);
  //   rooms.forEach(async room => {
  //     try {

  //       const alive_ids = Object.keys(self.io.sockets.connected).map(
  //         k => self.io.sockets.connected[k].peer.user
  //       );
  //       console.log("currently alive ", alive_ids.length);
  //       const coll = (await mongodb).collection("clients");
  //       const all_ids_to_remove = (await coll
  //         .find({ sock_id: { $nin: alive_ids } }, { sock_id: 1, _id: 0 })
  //         .toArray()).map(e => e.addr);
  //       console.log(
  //         "difference between alive and dead ",
  //         all_ids_to_remove.length
  //       );

  //       all_ids_to_remove.forEach(socket_id=>{

  //       })
  //       setTimeout(sync, 5000);
  //     } catch (e) {
  //       console.error(e);
  //       setTimeout(sync, 500);
  //     }
  //   });
  // };
  // sync();
};

/**
 * Shutdown the Symple server.
 *
 * @public
 */

Symple.prototype.shutdown = function() {
  if (this.timer) {
    clearInterval(this.timer);
  }
  this.io.close();
  this.server.destroy();
};

/**
 * Initialize HTTP server if required.
 *
 * @private
 */

Symple.prototype.initHTTP = function() {
  // Create the HTTP/S server if the instance not already set
  if (!this.server) {
    this.server = (this.config.ssl && this.config.ssl.enabled
      ? // HTTPS
        https.createServer({
          key: fs.readFileSync(this.config.ssl.key),
          cert: fs.readFileSync(this.config.ssl.cert)
        })
      : // HTTP
        http.createServer()
    )
      //).listen(process.env.PORT || this.config.port);
      .listen(this.config.port);
  }
};

/**
 * Initialize the Socket.IO server.
 *
 * @private
 */

Symple.prototype.initSocketIO = function() {
  var self = this;

  // Bind the Socket.IO server with the HTTP server
  this.io = sio.listen(this.server);

  // Handle socket connections
  this.io.on("connection", function(socket) {
    console.log("connect", socket.id);
    self.onConnection(socket);
  });
};

/**
 * Initialize the Redis adapter if required.
 *
 * @private
 */

Symple.prototype.initRedis = function() {
  var self = this;

  if (this.config.redis && !this.pub && !this.sub) {
    console.log("redis ok");
    var redis = require("redis").createClient;
    var opts = {};
    if (this.config.redis.password) {
      opts["pwd"] = this.config.redis.password;
    }
    this.pub = redis(this.config.redis.port, this.config.redis.host, opts);
    opts["return_buffers"] = true;
    this.sub = redis(this.config.redis.port, this.config.redis.host, opts);
    this.io.adapter(
      adapter({ key: "symple", pubClient: this.pub, subClient: this.sub })
    );

    // Touch sessions every `sessionTTL / 0.8` minutes to prevent them from expiring.
    if (this.config.sessionTTL > 0) {
      this.timer = setInterval(function() {
        debug("touching sessions");
        for (var i = 0; i < self.io.sockets.sockets.length; i++) {
          var socket = self.io.sockets.sockets[i];
          self.touchSession(socket.token, function(err, res) {
            debug(socket.id, "touching session:", socket.token, !!res);
          });
        }
      }, (this.config.sessionTTL / 0.8) * 60000);
    }
  }
};

/**
 * Called upon client socket connected.
 *
 * @param {Socket} socket - client socket
 * @private
 */

Symple.prototype.onConnection = function(socket) {
  debug(socket.id, "connection");
  var self = this,
    peer,
    timeout;

  // Give the client 2 seconds to `announce` or get booted
  timeout = setTimeout(function() {
    console.log(socket.id, "failed to announce");
    socket.disconnect();
  }, 2000);

  // Handle the `announce` request
  socket.on("announce", function(req, ack) {
    //console.log("announce", req);
    // Authorize the connection
    self.authorize(socket, req, async function(status, message) {
      debug(socket.id, "announce result:", status);
      console.log(socket.id, "announce result:", status);
      clearTimeout(timeout);
      peer = socket.peer;

      // Authorize response
      if (status == 200) {
        let d = peer.toPeer();
        d.iceServers = await makeIceArray();

        self.respond(ack, status, message, d);
      } else {
        self.respond(ack, status, message);
        console.log("disconnect auth", socket.id);
        socket.disconnect();
        return;
      }

      // Message
      socket.on("message", function(m, ack) {
        // debug('got message', m)
        if (m) {
          // if (m.type == 'presence')
          //   peer.toPresence(m);
          if (m.type == "presence") {
            //console.log("presence", m);
            peer.toPresence(m);
          }
          self.broadcast(socket, m);
          self.respond(ack, 200);
        }
      });

      // Dynmic rooms
      // TODO: Redis permission checking and Room validation
      if (self.config.dynamicRooms) {
        debug(socket.id, "enabling dynamic rooms");

        // Join Room
        socket.on("join", async function(room, ack) {
          console.log(socket.id, "joining room", room);
          if (!Rooms[room]) {
            await (await mongodb).addIdx(room);
            Rooms[room] = true;
          }
          if (Critical[room] === undefined) {
            Critical[room] = false;
          }
          if (!socket.rooms2) socket.rooms2 = {};
          if (!socket.rooms2[room]) {
            socket.join(room, function(err) {
              if (Del[socket.id+room]) return;
              if (err) {
                debug(socket.id, "cannot join room", err);
                self.respond(ack, 404, "Cannot join room: " + err);
              } else {
                let presenceMessage = peer.toPresence();
                //console.log(presenceMessage);
                presenceMessage.roomId = room;
                self.broadcastInRoom(socket, presenceMessage);
                //self.broadcastFromRoom(socket, room);
                self.respond(ack, 200, "Joined room: " + room);
              }
            });
            socket.rooms2[room] = true;
          }
        });

        // Leave Room
        socket.on("leave", function(room, ack) {
          debug(socket.id, "leaving room", room);
          if (!socket.rooms2) socket.rooms2 = {};
          delete socket.rooms2[room];
          // console.log(socket.id, "leaving room", room);
          // Clients = Clients.filter(c => c.id != socket.id);
          // Clients.forEach((c, index, arr) => {
          //   arr[index].connects = arr[index].connects.filter(
          //     cc => cc.id != socket.id
          //   );
          // });
          socket.leave(room, function(err) {
            if (err) {
              debug(socket.id, "cannot leave room", err);
              self.respond(ack, 404, "Cannot leave room: " + err);
            } else {
              self.respond(ack, 200, "Left room: " + room);
            }
          });
        });
      }

      // Peers
      // socket.on('peers', function(ack) {
      //  self.respond(ack, 200, '', self.peers(false));
      // });
    });
  });

  // Handle socket disconnection
  socket.on("disconnect", function() {
    console.log("i am dosconnecktink", socket.id);
    self.onDisconnect(socket);
  });
};

const waitSecs = secs =>
  new Promise(resolve => {
    setTimeout(resolve, secs * 1000);
  });

let Del = {};
/**
 * Called upon client socket disconnect.
 *
 * @param {Socket} socket - client socket
 * @private
 */

Symple.prototype.onDisconnect = function(socket) {
  debug(socket.id, "is disconnecting");

  if (socket.peer) {
    //console.log(socket.peer);
    // Broadcast offline presence
    if (socket.peer.online) {
      socket.peer.online = false;
      var p = socket.peer.toPresence();
      this.broadcast(socket, p);
    }

    // Leave rooms

    let self = this;
    console.log(socket.rooms2);
    let rooms = Object.keys(socket.rooms2 || {});
    const SOCKID = socket.id;
    const free = async room => {
      if (Del[SOCKID+room]) {
        return;
      } else {
        Del[SOCKID+room] = true;
      }

      if (Critical[room]) {
        Del[SOCKID+room] = false;
        await waitSecs(0.5);
        return free(room);
      }
      Critical[room] = true;
      const coll = (await mongodb).collection("clients" + room);
      let deleteresult = await coll.deleteOne({ addr: SOCKID });

      if (deleteresult.result.n != 1) {
        Critical[room] = false;
        Del[SOCKID+room] = false;
        await waitSecs(0.5);
        return free(room);
      }

      const tmpaggrs = await coll
        .aggregate([
          { $match: { connects: socket.peer.user } },
          {
            $addFields: {
              newnconnects: {
                $filter: {
                  input: "$connects",
                  as: "connect",
                  cond: { $not: [{ $eq: ["$$connect", socket.peer.user] }] }
                }
              }
            }
          }
        ])
        .toArray();
      for (let i = 0; i < tmpaggrs.length; ++i) {
        await coll.update(
          { _id: tmpaggrs[i]._id },
          {
            $set: { nconnects: tmpaggrs[i].newnconnects.length },
            $pull: { connects: { $nin: tmpaggrs[i].newnconnects } }
          }
        );
      }
      for (let i = 0; i < tmpaggrs.length; ++i) {
        if (self.io.sockets.connected[tmpaggrs[i].addr]) {
          const cl = await pickNodeToConnect(
            tmpaggrs[i],
            room
          );
          if (!cl) continue;
          await coll.findOneAndUpdate(
            { sock_id: tmpaggrs[i].addr },
            {
              $inc: { nconnects: 1 },
              $push: { connects: { $each: [cl.sock_id] } }
            }
          );          
          if(self.io.sockets.connected[tmpaggrs[i].addr]){
            let m = self.io.sockets.connected[tmpaggrs[i].addr].peer.toPresence();
            m.roomId = room;
            self.io.sockets.connected[tmpaggrs[i].addr].broadcast.json.to(cl.addr).send(m);
          }
          //self.broadcastInRoom(self.io.sockets.connected[tmpaggrs[i].addr], m);
        }
      }
      delete Del[SOCKID+room];
      Critical[room] = false;
    };
    rooms.forEach(async room => {
      console.log("room", room);
      await free(room);
    });
    socket.leave(socket.peer.user);
  }
};

Symple.prototype.pickClient = function(arrclients, client) {
  let min = 99999;
  let minind = -1;
  for (let i = 0; i < arrclients.length; ++i) {
    if (
      arrclients[i].nconnects < min &&
      !client.connects.some(c => c.id == arrclients[i].id)
    ) {
      min = arrclients[i].nconnects;
      minind = i;
    }
  }
  return minind;
};

/**
 * Called to authorize a new connection.
 *
 * @param {Socket} socket - client socket
 * @param {Object} data - arbitrary peer object
 * @param {function} fn - callback function
 * @private
 */

Symple.prototype.authorize = function(socket, req, fn) {
  var self = this;

  // Authenticated access
  if (self.config.authentication) {
    if (!req.user || !req.token) {
      return fn(401, "Authentication failed: Missing user or token param");
    }

    // Retreive the session from Redis
    debug(socket.id, "authenticating", req);
    self.getSession(req.token, function(err, session) {
      if (err) {
        return fn(401, "Authentication failed: Invalid session");
      }
      if (typeof session !== "object") {
        return fn(401, "Authentication failed: Session must be an object");
      } else {
        debug(socket.id, "authentication success");
        socket.token = req.token;
        self.authorizeValidPeer(socket, self.extend(req, session), fn);
      }
    });
  }

  // Anonymous access
  else {
    if (!req.user) {
      return fn(401, "Authentication failed: Missing user param");
    }

    this.authorizeValidPeer(socket, req, fn);
  }
};

/**
 * Create a valid peer object or return null.
 *
 * @param {Socket} socket - client socket
 * @param {Object} data - arbitrary peer object
 * @param {function} fn - callback function
 * @private
 */

Symple.prototype.authorizeValidPeer = function(socket, data, fn) {
  var peer = new Peer(
    this.extend(data, {
      id: socket.id,
      online: true,
      rtmpUrls: ss.get(),
      name: data.name || data.user,
      host:
        socket.handshake.headers["x-real-ip"] ||
        socket.handshake.headers["x-forwarded-for"] ||
        socket.handshake.address
    })
  );

  if (peer.valid()) {
    peer.connects = [];
    socket.join(peer.user); // Join user channel
    // socket.join('group-' + peer.group);   // Join group channel
    socket.peer = peer;

    debug(socket.id, "authentication success", peer);
    this.onAuthorize(socket);
    return fn(200, "Welcome " + peer.name);
  } else {
    debug(socket.id, "authentication failed: invalid peer object", peer);
    return fn(401, "Invalid peer session");
  }
};

/**
 * Create a valid peer object or return null.
 *
 * @param {Socket} socket - client socket
 * @param {Peer} peer - arbitrary peer object
 * @private
 */

Symple.prototype.onAuthorize = function(socket, peer) {
  // nothing to do
};

/**
 * Broadcast a message over the given socket.
 *
 * @param {Socket} socket - client socket
 * @param {Object} message - message to send
 * @public
 */

Symple.prototype.broadcast = function(socket, message) {
  if (!message || typeof message !== "object" || !message.from) {
    debug(socket.id, "dropping invalid message:", message);
    return;
  }

  debug(socket.id, "broadcasting message:", message);
  // Get an destination address object for routing
  var to = message.to;
  if (to === undefined) {
    //do nothing if not specifed to whom
    //socket.broadcast.json.send(message);
  } else if (typeof to === "string") {
    var addr = this.parseAddress(to.replace(message.roomId, ""));
    socket.broadcast.json.to(addr.user || addr.id).send(message); //.except(this.unauthorizedIDs())
  } else if (Array.isArray(to)) {
    // send to an multiple rooms
    for (var room in to) {
      socket.broadcast.json.to(to[room]).send(message);
    }
  } else {
    debug(socket.id, "cannot route message", message);
  }
};

Symple.prototype.broadcastInRoom = async function(socket, message) {
  if (!this.io.sockets.connected[socket.id]) {
    //console.log("broad", socket.id);
    return;
  }
  if (!message || typeof message !== "object" || !message.roomId) {
    debug(socket.id, "dropping invalid message:", message);
    return;
  }
  debug(
    socket.id,
    "broadcasting message:",
    message,
    ", in room:",
    message.roomId
  );

  // Get an destination address object for routing
  var room = message.roomId;

  if (Del[socket.id+room]) {
    //console.log("Del", socket.id);
    return;
  }

  let client = {
    addr: socket.id,
    connects: [],
    nconnects: 0,
    sock_id: socket.peer.user
  };

  let sendTo = [];

  if (Critical[room]) {
    let self = this;
    setTimeout(() => {
      self.broadcastInRoom(socket, message);
    }, 500);
    return;
  }

  Critical[room] = true;

  for (let iterations = 2; iterations > 0; --iterations) {
    let cl = await pickNodeToConnect(client, room);
    if (!cl) break;
    client.connects.push(cl.sock_id);
    ++client.nconnects;
    sendTo.push(cl.addr);
  }

  let coll = (await mongodb).collection("clients" + room);

  const upd = await coll.findOneAndUpdate(
    { sock_id: client.sock_id },
    {
      $inc: { nconnects: client.connects.length },
      $push: { connects: { $each: client.connects } }
    }
  );

  if (!upd.value) {
    await coll.insertOne(client);
  }

  Critical[room] = false;

  if (!this.io.sockets.connected[socket.id] || Del[socket.id+room]) return;
  sendTo.map(toId => {
    if (!this.io.sockets.connected[toId]) return;
    socket.broadcast.json.to(toId).send(message);
  });
};

Symple.prototype.broadcastFromRoom = function(socket, room) {
  if (!room || typeof room !== "string") {
    debug(socket.id, "dropping invalid message in broadcats from room:", room);
    return;
  }

  debug(socket.id, "broadcasting everyone presence from room:", room);

  // Get an destination address object for routing
  var to = socket.id;
  let socketIds = this.io.sockets.adapter.rooms[room].sockets;
  if (socketIds) {
    for (let id in socketIds) {
      let socket = this.io.sockets.connected[id];
      let m = socket.peer.toPresence();
      this.io.json.to(to).send(m);
    }
  } else {
    console.log(sockets);
  }
  return;
};

Symple.prototype.respond = function(ack, status, message, data) {
  if (typeof ack !== "function") return;
  var res = {};
  res.type = "response";
  res.status = status;
  if (message) res.message = message;
  if (data) res.data = data.data || data;
  debug("responding", res);
  ack(res);
};

/**
 * Get a peer session by it's token.
 *
 * @param {Object} token - address string
 * @param {function} fn - callback function
 * @public
 */

Symple.prototype.getSession = function(token, fn) {
  this.pub.get("symple:session:" + token, function(err, reply) {
    if (reply) {
      fn(err, JSON.parse(reply));
    } else fn("No session", null);
  });
};

/**
 * Touch a peer session by it's token and extend
 * it's lifetime by (config.sessionTTL) minutes.
 *
 * @param {Object} token - address string
 * @param {function} fn - callback function
 * @public
 */

Symple.prototype.touchSession = function(token, fn) {
  if (this.config.sessionTTL == -1) return;

  var expiry = parseInt(+new Date() / 1000) + this.config.sessionTT * 60;
  this.pub.expireat("symple:session:" + token, expiry, fn);
};

/**
 * Parse a peer address with the format: user|id
 *
 * @param {Object} str - address string
 * @public
 */

Symple.prototype.parseAddress = function(str) {
  var addr = {};
  arr = str.split("|");

  if (arr.length < 2)
    // no id
    addr.user = arr[0];
  else {
    // has id
    addr.user = arr[0];
    addr.id = arr[1];
  }

  return addr;
};

Symple.prototype.getSocketsByRoom = function(nsp, room) {
  var users = [];
  for (var id in this.io.of(nsp).adapter.rooms[room]) {
    users.push(this.io.of(nsp).adapter.nsp.connected[id]);
  }
  return users;
};

/**
 * Extend an object with another object.
 *
 * @param {Object} origin - target object
 * @param {Object} add - source object
 * @private
 */

Symple.prototype.extend = function(origin, add) {
  // Don't do anything if add isn't an object
  if (!add || typeof add !== "object") return origin;

  var keys = Object.keys(add);
  var i = keys.length;
  while (i--) {
    origin[keys[i]] = add[keys[i]];
  }
  return origin;
};

/**
 * Module exports.
 */

module.exports = Symple;
