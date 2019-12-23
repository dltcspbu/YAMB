"use strict";

const { ServiceProvider } = require("@adonisjs/fold");

class AppProvider extends ServiceProvider {
  _sympleSignaller() {
    this.app.singleton("sympleSignaller", () => {
      let webrtc = require("../src/app");
      return webrtc;
    });
  }

  _rtmpUrls() {
    this.app.singleton("rtmpUrls", () => {
      var obj = {};
      obj.urls = [];
      return {
        add: url => {
          if (!obj.urls.includes(url)) obj.urls.push(url);
        },
        get: () => {
          return obj.urls;
        },
        remove: url => {
          if (obj.urls.includes(url))
            obj.urls = obj.urls.filter(elem => {
              return !elem.includes(url)
            })
        }
      };
    });
  }

  _mongo(){
    this.app.singleton("mongodb", () => {
      let mongodb = require("../src/mongodb");
      return mongodb;
    });
  }

  register() {
    this._mongo()
    this._sympleSignaller();
    this._rtmpUrls();
  }

  boot() {
    const ss = use("sympleSignaller");
    ss.init();
    const rtmp = use("rtmpUrls");
  }
}

module.exports = AppProvider;
