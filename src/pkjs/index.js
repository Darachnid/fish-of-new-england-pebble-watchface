var Clay = require("@rebble/clay");
var clayConfig = require("./config");

// Clay registers showConfiguration / webviewclosed and sends settings via AppMessage.
new Clay(clayConfig);