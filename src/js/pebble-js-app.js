function shorten(text) {
    if (!text || text.length < 1) {
        return " ";
    }
    if (text.length > 20) {
        return text.substring(0,19) + "...";
    }
    return text;
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  var response;
  var req = new XMLHttpRequest();
  var uri = "http://prompt.eps0.net/nextappt?" + "lat=" + coordinates.latitude + "&lon=" + coordinates.longitude + "&pebble_id=1" 
  req.open('GET', uri, true);
  req.onreadystatechange = function(e) { //onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log("nextappt: " + req.responseText);
        response = JSON.parse(req.responseText);
        if (response) {
          var nextappt = response;
          var appt_name = shorten(nextappt.next_appt_name);
          var route_name = shorten(nextappt.route_name);
          var travel_method = shorten(nextappt.travel_method);
          var now = Math.round(new Date().getTime()/1000);
          var leave_in = nextappt.departure_time - now;
          console.log("deptime : " + nextappt.departure_time);
          console.log("now     : " + now);
          console.log("leave in: " + leave_in);
          payload = {
            "next_appt_name": appt_name,
            "next_appt_time": nextappt.next_appt_time,
            "travel_time":    nextappt.travel_time,
            "travel_method":  travel_method,
            "route_name":     route_name,
            "departure_time": nextappt.departure_time,
            "leave_in": leave_in
          };
          console.log("sending_payload: " + JSON.stringify(payload));
          Pebble.sendAppMessage(payload);
        }
      } else {
        console.log("Error Status: " + req.statusText + "\n" + req.response);
      }
    } else {
        console.log("req.readyState:" + req.readyState + " status: " + req.status);
    }
  }
  console.log("requesting:" + uri);
  req.send(null);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "next_appt_name":"Next Appt Unavailable",
  });
}

Pebble.addEventListener("ready",
    function(e) {
        console.log("Prompt ready.");
    }
);

var locationOptions = { "timeout": 5000, "maximumAge": 60000 }; // 5 sec timeout, max age 1 min

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log("received: " + e.type);
                          console.log("rpayload: " + JSON.stringify(e.payload));
                        });

