"use strict";
const ss = use("rtmpUrls");
const execa = require("execa");

const mongodb = use("mongodb")()

const wait = secs => new Promise(resolve=>setTimeout(resolve, secs))
class ChannelController {
  async add({ request, response }) {
    const url = request.only(["rtmp"]).rtmp;
    if (!url || !url.includes("://")) return response.redirect("/add");
    let ex = execa.command(
      `ffprobe -v quiet -print_format json -show_streams ${url}`
    );
    let count = 20;
    while(count > 0) {
    --count
    let indicator = String(ex.stdout.write)
    if (!indicator.includes('writeAfterFIN') && count == 0) {
      response.redirect("/add");
    } else if (indicator.includes('writeAfterFIN')) {
      try {
        await ex
      } catch(e) {
        return response.redirect("/add");
      }
      ss.add(url);
      use("sympleSignaller").onAddRoom(url);
      response.redirect("/add");
      break;
    }
    await wait(500)
    }
    ex.kill('SIGTERM', {
      forceKillAfterTimeout: 2000
      });
  }

  remove({ request, response, view }) {
    const url = request.only(["rtmp"]).rtmp;
    ss.remove(url);
    use("sympleSignaller").onRemoveRoom(url);
    return response.redirect("/add");
  }

  async remove_ice_server({ request, response, view }) {
    const iceserver = request.only(["iceserver"]).iceserver;
    const sobakaSplit = iceserver.split('@')
    const splitFirstPart = sobakaSplit[0].split(':')
    const type = splitFirstPart[0]
    const owner = splitFirstPart[1]
    if (!owner || owner.length < 1)
      return response.json({e:"Owner is required"});
    const secret = splitFirstPart[2] || ""
    const auth = secret.length > 0
    const splitSecondPart = sobakaSplit[1].split(':')
    const host = splitSecondPart[0]
    const port = splitSecondPart[1]
    const iceserverModel = {type, owner, secret, auth, host, port}
    ;(await mongodb).IceServers.delete(iceserverModel)
    return response.redirect("/add_ice_server");
  }

  async add_ice_server({ request, response, view }) {
    const iceserver = request.only(["iceserver"]).iceserver;
    const sobakaSplit = iceserver.split('@')
    const splitFirstPart = sobakaSplit[0].split(':')
    const type = splitFirstPart[0]
    const owner = splitFirstPart[1]
    if (!owner || owner.length < 1)
      return response.json({e:"Owner is required"});
    const secret = splitFirstPart[2] || ""
    const auth = secret.length > 0
    const splitSecondPart = sobakaSplit[1].split(':')
    const host = splitSecondPart[0]
    const port = splitSecondPart[1]
    const iceserverModel = {type, owner, secret, auth, host, port}
    ;(await mongodb).IceServers.add(iceserverModel)
    return response.redirect("/add_ice_server");
  }

  delete({ request, response }) {
    const url = request.only(["rtmp"]).rtmp;
    if (ss.get(url)) {
    }
    ss.add(url);
    use("sympleSignaller").onAddRoom(url);
  }
}

module.exports = ChannelController;
