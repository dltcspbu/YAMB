const Env = use("Env");
const MongoClient = require("mongodb").MongoClient;

// Connection URL
const url = Env.get("MONGO_URL", "mongodb://172.27.27.43:8703");

// Database Name
const dbName = "reestr";

let db = null;

const restarter = () => {
  MongoClient.connect(url, function(err, client) {
    if (err) {
      setTimeout(restarter, 1000);
    }

    client
      .db(dbName)
      .dropDatabase()
      .then(() => {
        client.db(dbName).addIdx = room => {
          db.collection("clients" + room).createIndex({ sock_id: -1 });
          db.collection("clients" + room).createIndex({ connects: -1 });
          db.collection("clients" + room).createIndex({ nconnects: -1 });
          db.collection("clients" + room).createIndex({ addr: -1 });
        };

        client.db(dbName).IceServers = {
          get: async () => {
            let tmp = await db
              .collection("iceServers")
              .find({})
              .toArray();
            return (await db
              .collection("iceServers")
              .find({})
              .toArray()).map(s=>{
                return s.type+":"+s.owner+":"+s.secret+"@"+s.host+":"+s.port;
              });
          },
          add: async ({ type, owner, secret, auth, host, port }) => {
            const iceServer = {
              url: `${type}:${host}:${port}`,
              owner,
              secret,
              auth,
              host,
              port,
              type
            };
            db.collection("iceServers").findOneAndUpdate(
              { url: iceServer.url },
              {
                $setOnInsert: iceServer
              },
              {
                returnOriginal: true,
                upsert: true
              }
            );
          },
          delete: async ({ type, owner, secret, auth, host, port }) => {
            db.collection("iceServers").remove({ url: `${type}:${host}:${port}`, owner, secret });
          }
        };

        db = client.db(dbName);
      });

    //client.close();
  });
};

restarter();

const get = function() {
  return new Promise((resolve, reject) => {
    if (db) {
      resolve(db);
    } else {
      setTimeout(() => resolve(get()), 250);
    }
  });
};

module.exports = get;
