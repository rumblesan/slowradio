/* global JSONP */

var getdata = function () {
  JSONP({
    url: 'https://proxima.shoutca.st/external/rpc.php',
    data: {
      m: 'streaminfo.get',
      username: 'slowradio',
      charset: '',
      mountpoint: '',
      rid: 'slowradio'
    },
    success: function (result) {
      var data = result.data[0];
      document.getElementById('song-info').innerHTML = data.song;
    }
  });
};

getdata();
setInterval(getdata, 60 * 1000);
