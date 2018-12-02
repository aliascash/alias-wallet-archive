var sendPage;
$(function() {
  sendPage = function() {
    function ready() {
      callback(false);
      initialize();
      toggle();
      $("#send [name^=transaction_type]").on("change", toggle);
      $("#send [data-toggle=tab]").on("shown.bs.tab", toggle);
    }
    function init(options) {
      return options ? 2 == options.at && ("" === $("#send-balance .pay_to").val() && ($("#send-balance .pay_to").val(options.address).change(), $("#send-balance .pay_to").data("address", options.address))) : $("#send-balance .pay_to").val($("#send-balance .pay_to").data("address")).change(), true;
    }
    function callback(active) {
      if (void 0 === active) {
        active = $(".show-coin-control .btn-cons").hasClass("active");
      }
      $("#coincontrol").toggle(active);
    }
    function onSuccess() {
      if ($("#coincontrol").is(":visible")) {
        var r20 = 0;
        var errorClass = 0;
        for (;errorClass < recipients;errorClass++) {
          r20 += unit.parse($("#amount" + errorClass).val());
        }
        bridge.updateCoinControlAmount(r20);
      }
    }
    function color(source, type, data, name, to, text, el, key) {
      if ($("#coincontrol").is(":visible")) {
        $("#coincontrol_auto").toggle(0 === source);
        $("#coincontrol_labels").toggle(source > 0);
        if (source > 0) {
          $("#coincontrol_quantity").text(source);
          $("#coincontrol_amount").text(unit.format(type));
          $("#coincontrol_fee").text(unit.format(data));
          $("#coincontrol_afterfee").text(unit.format(name));
          $("#coincontrol_bytes").text("~" + to).css("color", to > 1E4 ? "red" : null);
          $("#coincontrol_priority").text(text).css("color", 0 == text.indexOf("low") ? "red" : null);
          $("#coincontrol_low").text(el).toggle(key).css("color", "yes" == el ? "red" : null);
          $("#coincontrol_change").text(unit.format(key)).toggle(key);
          $("label[for='coincontrol_low'],label[for='coincontrol_change']").toggle(key);
        } else {
          $("#coincontrol_quantity").text("");
          $("#coincontrol_amount").text("");
          $("#coincontrol_fee").text("");
          $("#coincontrol_afterfee").text("");
          $("#coincontrol_bytes").text("");
          $("#coincontrol_priority").text("");
          $("#coincontrol_low").text("");
          $("#coincontrol_change").text("");
        }
      }
    }
    function parent() {
      return $("div.recipient").length;
    }
    function initialize() {
      $("#recipients").append(((0 == parent() ? "" : "<hr />") + s.replace(/recipient-template/g, "recipient[count]")).replace(/\[count\]/g, ++num));
      $("#recipient" + num.toString() + " [data-title]").tooltip();
      $("#amount" + num.toString()).on("keydown", unit.keydown).on("paste", unit.paste);
      bridge.userAction(["clearRecipients"]);
      // show public or private unit depending on transaction_type_to
      var trxTypeTo = $("[name=transaction_type_to]:checked").val();
      $(".to_unit_public").css('display', 'public' === trxTypeTo ? 'block': 'none');
      $(".to_unit_private").css('display', 'private' === trxTypeTo ? 'block': 'none');
    }
    function draw(time, value, v, hour) {
      reset();
      $("#recipient" + num.toString() + " .pay_to").val(time).change();
      $("#recipient" + num.toString() + " .pay_to_label").val(value).change();
      $("#recipient" + num.toString() + " .amount").val(hour).change();
      $("#recipient" + num.toString() + " .narration").val(v).change();
      $("[href=#send]").click();
    }
    function reset() {
      $("#recipients").html("");
      $("#send-balance .amount").val("0").change();
      initialize();
      init();
      resetGlobalVariables();
    }
    function setup(element) {
      if (parent() <= 1) {
        reset();
      } else {
        element = $(element);
        if (0 == element.next("hr").remove().length) {
          element.prev("hr").remove();
        }
        element.remove();
        $("#tooltip").remove();
      }
    }
      function resetGlobalVariables() {
          numOfRecipients = 0;
      }

      function sendCoinsClicked() {
          resetGlobalVariables();
          update();
      }

    function update(sendCoinsResult, addRecipientResult) {
        function addRecipient() {
            numOfRecipients++;

            var context = $(this).find(".pay_to");
            var jElm = $(this).find(".amount");

            var narration = $(this).find(".narration").val();
            // Fix if narration is undefined, addRecipient was never called on QT bridge (QT 5.9.6)
            if (typeof narration === 'undefined') {
                narration = "";
            }
            bridge.addRecipient(context.val(), $(this).find(".pay_to_label").val(), narration, unit.parse(jElm.val(), $(this).find(".unit").val()), datas, $("#ring_size").val());
        }

        if (typeof sendCoinsResult !== 'undefined') {
            if (sendCoinsResult) {
                reset();
            }
        }
        else if (typeof addRecipientResult !== 'undefined' ) {
            if (addRecipientResult) {
                numOfRecipients--;
            }
            if (numOfRecipients == 0) {
                bridge.sendCoins($("#coincontrol").is(":visible"), $("#change_address").val())
            }
        }
        else {
            var datas = load();

            bridge.userAction(["clearRecipients"]);

            if ($("#send-balance").is(":visible")) {
                $("#send-balance").each(addRecipient);
            } else {
                $("div.recipient").each(addRecipient);
            }
        }
    }
    function toggle(e) {
      var isSendMain = $("#send-main").is(":visible");
      var trxTypeFrom = $("[name=transaction_type_from]:checked").val();
      var trxTypeTo = $("[name=transaction_type_to]:checked").val();
        console.log('OPEN');
        console.log(trxTypeFrom)
      if (e) {
        if (e.target !== $("input#to_account_public")[0] && e.target !== $("input#to_account_private")[0]) {
            trxTypeTo = isSendMain ? trxTypeFrom : "public" === trxTypeFrom ? "private" : "public";
            $("input[name=transaction_type_to][value=" + trxTypeTo + "]").prop("checked", true);
        }
        else if (!isSendMain) {
            trxTypeFrom = "public" === trxTypeTo ? "private" : "public";
            $("input[name=transaction_type_from][value=" + trxTypeFrom + "]").prop("checked", true);
        }
      }
      var a = $("[name=transaction_type_to  ]:checked").val();
      var ok = load();
      $("#spend_spec").toggle("public" === trxTypeFrom);
      $("#spend_spectre").toggle("private" === trxTypeFrom);
      $("#to_spec").toggle("public" === a);
      $("#to_spectre").toggle("private" === a);
      $("#to_balance").toggle(!isSendMain);
      $(".show-coin-control").toggle(ok < 1);
      sendPage.toggleCoinControl(ok < 0);

      $(".advanced_controls").toggle(!isSendMain);
      $("#add_recipient").toggle($("#send-main").is(":visible"));

      $(".to_unit_public").css('display', 'public' === trxTypeTo ? 'block': 'none');
      $(".to_unit_private").css('display', 'private' === trxTypeTo ? 'block': 'none');

      if (!isSendMain) {
        init();
      }
    }
    function load() {
      var arrayLike = "public" === $("[name=transaction_type_from]:checked").val();
      var value = "public" === $("[name=transaction_type_to  ]:checked").val();
      return arrayLike ? +!value : 2 + value;
    }
    var s = $("#recipient-template")[0].outerHTML;
    var num = 0;
    return $("#recipient-template").remove(), {
      init: ready,
      initSendBalance: init,
      toggleCoinControl: callback,
      addRecipient: initialize,
      addRecipientDetail: draw,
      clearRecipients: reset,
      removeRecipient: setup,
      sendCoins: sendCoinsClicked,
      update: update,
      updateCoinControl: onSuccess,
      updateCoinControlInfo: color,
      changeTransactionType: toggle
    };
  }();
  sendPage.init();
});
