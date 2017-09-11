var showQRCode;

$(function() {
  function parse(resp, str) {
    if (void 0 !== resp) {
      opts.val(resp);
    }
    if (void 0 !== str) {
      txt.val(str);
    }
    handler.clear();
    var e = "spectrecoin:" + opts.val() + "?label=" + txt.val() + "&narration=" + $radio.val() + "&amount=" + unit.parse($("#qramount").val(), $("#qrunit").val());
    errors.text(e);
    handler.makeCode(e);
  }
  var handler = new QRCode("qrcode", {
    colorDark: "#E51C39",
    colorLight: "#ffffff",
    correctLevel: QRCode.CorrectLevel.H,
    width: 220,
    height: 220
  });
  var errors = $("#qrcode-data");
  var opts = $("#qraddress");
  var txt = $("#qrlabel");
  var $radio = $("#qrnarration");
  showQRCode = parse;

  $("#qramount").on("keydown", unit.keydown).on("paste", unit.paste);
});
