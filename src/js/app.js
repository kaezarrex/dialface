var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(JSON.parse(this.responseText));
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://forecast-proxy.ocean.dhaz.in/' +
      pos.coords.latitude + ',' + pos.coords.longitude + '?units=si&exclude=flags,minutely,hourly';

  // Send request
  xhrRequest(url, 'GET', function(response) {
    console.log(response.currently.temperature, response.latitude, response.longitude);
    Pebble.sendAppMessage(
      {
        'AppKeyTemperature': response.currently.temperature,
        'AppKeySunrise': response.daily.data[0].sunriseTime,
        'AppKeySunset': response.daily.data[0].sunsetTime,
      },
      function(e) { },
      function(e) { console.log('Error sending weather info to Pebble!'); }
    );
  });
}

function locationError(err) {
  if(err.code == err.PERMISSION_DENIED) {
    console.log('Location access was denied by the user.');  
  } else {
    console.log('location error (' + err.code + '): ' + err.message);
  }
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 15000}
  );
}

Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');
  }
);

setInterval(function() {
  if ((new Date()).getMinutes() % 5 === 0) getWeather();
}, 60 * 1000);