module.exports = function(minified) {
  var Clay = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  Clay.on(Clay.EVENTS.AFTER_BUILD, function() {
    var connection = new WebSocket("wss://liveconfig.fletchto99.com/forward/" + Clay.meta.accountToken + "/" + Clay.meta.watchToken);
    connection.onopen = function() {
      Clay.getAllItems().map( function(item) {
        item.on('change', function() {
          connection.send(JSON.stringify({id: this.appKey, value: this.get()}));
        });
      });
    };
    Clay.getItemsByType('color').map( function(item) {
      var color = item.get();
      var r = Math.floor((((color >> 16) & 0xFF) + 42) / 85) * 85;
      var g = Math.floor((((color >>  8) & 0xFF) + 42) / 85) * 85;
      var b = Math.floor((((color >>  0) & 0xFF) + 42) / 85) * 85;
      item.set((r << 16) + (g << 8) + b);
    });
  });
};