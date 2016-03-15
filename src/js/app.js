var Clay = require('clay');
var clayConfig = require('config.json');
var customClay = require('custom-clay');
var clay = new Clay(clayConfig, customClay, {autoHandleEvents: false});

Pebble.addEventListener('showConfiguration', function(e) {
  var websocket = new WebSocket("wss://liveconfig.fletchto99.com/receive/"+Pebble.getAccountToken()+"/"+Pebble.getWatchToken());
  websocket.onmessage = function(message) {
    var attr = JSON.parse(message.data);
    var config = {};
    config[attr.id] = attr.value;
    console.log(JSON.stringify(config));
    
    // save the setting in the localstorage
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    settings[attr.id] = attr.value;
    localStorage.setItem('clay-settings', JSON.stringify(settings));

    Pebble.sendAppMessage(config);
  };
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) { return; }

  // Send settings to Pebble watchapp
  Pebble.sendAppMessage(clay.getSettings(e.response), function(e) {
    console.log('Sent config data to Pebble');
  }, function() {
    console.log('Failed to send config data!');
    console.log(JSON.stringify(e));
  });
});