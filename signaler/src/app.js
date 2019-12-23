//
/// Setup the Symple server
var Symple = require('./symple/symple');
var sy = new Symple();
sy.loadConfig(__dirname + '/symple.json'); // see config.json for options

var RTMP_URLS = [];
module.exports = sy
console.log('Symple server listening on port ' + sy.config.port);
