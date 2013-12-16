var arduino = require('../');

var board = new arduino.Board({
  debug: true
});

var led = new arduino.Led({
  board: board,
  pin: "A0"
});

board.on('ready', function(){
  led.blink();
});
