'use strict'

/*
|--------------------------------------------------------------------------
| Routes
|--------------------------------------------------------------------------
|
| Http routes are entry points to your web application. You can create
| routes for different URLs and bind Controller actions to them.
|
| A complete guide on routing is available here.
| http://adonisjs.com/docs/4.1/routing
|
*/

/** @type {typeof import('@adonisjs/framework/src/Route/Manager')} */
const Route = use('Route')

Route.post('/adding', 'ChannelController.add')

Route.post('/removing', 'ChannelController.remove')

// Route.get('/check', () => {
//   const webrtcController = use('webrtc')
//   const urls = webrtcController.get()
//   return urls

// })

// Route.get('/logs', ({ request, response, view }) => {
//   return view.render('logs')
// })

Route.post('/deleting', 'ChannelController.delete')

// Route.get('/check', () => {
//   const webrtcController = use('webrtc')
//   const urls = webrtcController.get()
//   return urls

// })

// Route.get('/logs', ({ request, response, view }) => {
//   return view.render('logs')
// })

const ss = use('rtmpUrls')
const mongodb = use("mongodb")()

Route.get('/remove', ({ request, response, view }) => {
  return view.render('removechannel', {urls: ss.get()})
})

Route.post('/removing_ice_server', 'ChannelController.remove_ice_server')
Route.post('/adding_ice_server', 'ChannelController.add_ice_server')



Route.get('/add', ({ request, response, view }) => {
  return view.render('addchannel', {urls: ss.get()})
})

Route.get('/add_ice_server', async ({ request, response, view }) => {
  return view.render('addiceserver', {urls: (await (await mongodb).IceServers.get())})
})