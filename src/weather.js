var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // request weather here
  // construct url
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude;

  // send request
  xhrRequest(url, 'GET', function(response){
    //response contains a JSON object with weather info
    var json = JSON.parse(response);

    // Temperature is in Kelvin, adjust
    var temperature = Math.round(json.main.temp - 273.15);
    console.log('Temperature is: '+temperature);

    // conditions
    var conditions = json.weather[0].main;
    console.log('Conditions are: '+conditions);
    // assemble dictionary using our keys
    var dictionary = {
      'KEY_TEMPERATURE':temperature,
      'KEY_CONDITIONS':conditions
    };

    // send to pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {console.log('data sent successfully')},
      function (e) {console.log('sending error')}
    );

  });
}

function locationError(err) {
  console.log('Error requesting location');
}

function getWeather(){
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout:15000, maximumAge:60000}
  );
}

//listen for when the watchface is opened
Pebble.addEventListener('ready', function(e){
  console.log('ready');

  // get the initial weather
  getWeather();
});

//listen for when a message is recieved
Pebble.addEventListener('appmessage', function(e){
  console.log('recieved appmessage');
  getWeather();
});
