function invalid(name, color) {
  return color === true ? name.css("background", "").css("color", "") : name.css("background", "#155b9a").css("color", "white"), 1 == color;
}
function updateValue(button) {
  function complete(status) {
    var $field = $(".newval");
    if (0 !== $field.length) {
      button.html(names.replace(name, $field.val().trim()));
    }
  }
  var names = button.html();
  var name = void 0 !== button.parent("td").data("label") ? button.parent("td").data("label") : void 0 !== button.parent("td").data("value") ? button.parent("td").data("value") : void 0 !== button.data("label") ? button.data("label") : void 0 !== button.data("value") ? button.data("value") : button.text();
  var result = button.parents(".selected").find(".address");
  var selected = button.parents(".selected").find(".addresstype");
  result = result.data("value") ? result.data("value") : result.text();
  if (1 === selected.length) {
    selected = selected.data("value") ? selected.data("value") : selected.text();
  }
  button.html('<input class="newval" type="text" onchange="bridge.updateAddressLabel(\'' + result + '\', this.value);" value="' + name + '" size=60 />');
  $(".newval").focus().on("contextmenu", function(event) {
    event.stopPropagation();
  }).keyup(function(e) {
    if (13 == e.keyCode) {
      complete(e);
    }
  });
  $(document).one("click", complete);
}
var connectSignalsAttempts = 0;
function connectSignals() {
  if (typeof(bridge) == "undefined" || typeof(optionsModel) == "undefined" || typeof(walletModel) == "undefined") {
    connectSignalsAttempts += 1;
    if (connectSignalsAttempts < 10) {
      console.log("retrying connecting signals in 200ms");
      setTimeout(connectSignals, 200);
    }
    else {
      console.log("giving up on connecting signals.");
      console.log("bridge: "+bridge);
      console.log("optionsModel: "+optionsModel);
      console.log("walletModel: "+walletModel);
    }
    return;
  }

  bridge.emitPaste.connect(pasteValue);
  bridge.emitTransactions.connect(appendTransactions);
  bridge.emitAddresses.connect(appendAddresses);
  bridge.emitCoinControlUpdate.connect(updateCoinControlInfo);
  bridge.triggerElement.connect(triggerElement);
  bridge.emitReceipient.connect(addRecipientDetail);
  bridge.networkAlert.connect(networkAlert);
  bridge.getAddressLabelResult.connect(getAddressLabelResult);
  bridge.newAddressResult.connect(newAddressResult);
  bridge.lastAddressErrorResult.connect(lastAddressErrorResult);
  bridge.getAddressLabelForSelectorResult.connect(getAddressLabelForSelectorResult);

  blockExplorerPage.connectSignals();
  walletManagementPage.connectSignals();
  optionsPage.connectSignals();
  chainDataPage.connectSignals();


  bridge.validateAddressResult.connect(validateAddressResult);
  bridge.addRecipientResult.connect(addRecipientResult);
  bridge.sendCoinsResult.connect(sendCoinsResult);
  bridge.transactionDetailsResult.connect(transactionDetailsResult);

  optionsModel.displayUnitChanged.connect(unit_setType);
  optionsModel.reserveBalanceChanged.connect(updateReserved);
  optionsModel.rowsPerPageChanged.connect(updateRowsPerPage);
  optionsModel.visibleTransactionsChanged.connect(visibleTransactions);

  walletModel.encryptionStatusChanged.connect(encryptionStatusChanged);
  walletModel.balanceChanged.connect(updateBalance);

  overviewPage.clientInfo();
  optionsPage.update();
  chainDataPage.updateAnonOutputs();
  translateStrings();


  bridge.jsReady();
}

function transactionDetailsResult(result) {
    $("#transaction-info").html(result);
}

var numOfRecipients = undefined;

function validateAddressResult(result) {
    // not in use currently
}

function sendCoinsResult(result) {
    console.log('sendCoinsResult')
    console.log(result)
    sendPage.update(result, undefined);
}


function addRecipientResult(result) {
    sendPage.update(undefined, result);
}


function encryptionStatusChanged(status) {
    overviewPage.encryptionStatusChanged(status)
}

function unit_setType(unitType) {
    unit.setType(unitType)
}

function updateCoinControlInfo(quantity, amount, fee, afterfee, bytes, priority, low, change) {
    sendPage.updateCoinControlInfo(quantity, amount, fee, afterfee, bytes, priority, low, change)
}

function addRecipientDetail(address, label, narration, amount) {
    sendPage.addRecipientDetail(address, label, narration, amount)
}

function updateReserved(name) {
    overviewPage.updateReserved(name);
}

function updateBalance(ns, key, id, name, type) {
    overviewPage.updateBalance(ns, key, id, name, type);
}

function triggerElement($window, completeEvent) {
  $($window).trigger(completeEvent);
}
function updateRowsPerPage(pageSize) {
  $(".footable").each(function() {
    var $target = $(this);
    if (!$target.hasClass("footable-lookup")) {
      $target.data().pageSize = pageSize;
      $target.trigger("footable_initialize");
    }
  });
}
function pasteValue(coords) {
  $(pasteTo).val(coords);
}
function paste(vim) {
  pasteTo = vim;
  bridge.paste();
  if (!(0 != pasteTo.indexOf("#pay_to") && "#change_address" != pasteTo)) {
    base58.check(pasteTo);
  }
}
function copy(obj, v) {
  var message = "";
  try {
    message = $(obj).text();
  } catch (e) {
  }
  if (!(void 0 != message && void 0 == v)) {
    message = "copy" == v ? obj : $(obj).attr(v);
  }
  bridge.copy(message);
}
function networkAlert(time) {
  $("#network-alert span").text(time).toggle("" !== time);
}
function openContextMenu(connection) {
  if (contextMenus.indexOf(connection) === -1) {
    contextMenus.push(connection);
  }
  if (void 0 !== connection.isOpen) {
    if (1 === connection.isOpen) {
      connection.isOpen = 0;
      if (connection.close) {
        connection.close();
      }
    }
  }
  var i = 0;
  for (;i < contextMenus.length;++i) {
    contextMenus[i].isOpen = contextMenus[i] == connection ? 1 : 0;
  }
}
function receivePageInit() {
  var r20 = [{
    name : "Copy&nbsp;Address",
    fun : function() {
      copy("#receive .footable .selected .address");
    }
  }, {
    name : "Copy&nbsp;Label",
    fun : function() {
      copy("#receive .footable .selected .label2");
    }
  }, {
    name : "Copy&nbsp;Public&nbsp;Key",
    fun : function() {
      copy("#receive .footable .selected .pubkey");
    }
  }, {
    name : "Edit",
    fun : function() {
      $("#receive .footable .selected .label2.editable").dblclick();
    }
  }];
  $("#receive .footable tbody").on("contextmenu", function(ev) {
    $(ev.target).closest("tr").click();
  }).contextMenu(r20, {
    triggerOn : "contextmenu",
    sizeStyle : "content"
  });
  $("#filter-address").on("input", function() {
    var $table = $("#receive-table");
    if ("" === $("#filter-address").val()) {
      $table.data("footable-filter").clearFilter();
    }
    $("#receive-filter").val($("#filter-address").val() + " " + $("#filter-addresstype").val());
    $table.trigger("footable_filter", {
      filter : $("#receive-filter").val()
    });
  });
  $("#filter-addresstype").change(function() {
    var $table = $("#receive-table");
    if ("" === $("#filter-addresstype").val()) {
      $table.data("footable-filter").clearFilter();
    }
    $("#receive-filter").val($("#filter-address").val() + " " + $("#filter-addresstype").val());
    $table.trigger("footable_filter", {
      filter : $("#receive-filter").val()
    });
  });
}
function clearRecvAddress() {
  $("#new-address-label").val("");
  $("#new-addresstype").val(1);
}
function addAddress() {
    console.log('addAddress');
  var throughArgs = $("#new-addresstype").val();
  var r20 = $("#new-address-label").val();
  bridge.newAddress(r20, throughArgs, '', false);
}

function clearSendAddress() {
  $("#new-send-label").val("");
  $("#new-send-address").val("");
  $("#new-send-address-error").text("");
  $("#new-send-address").removeClass("inputError");
}

function getAddressLabelForSelectorResult(result, selector, fallback) {
    if (!result) {
      result = fallback;
    }
    $(selector).val(result).text(result).change();
}

function addSendAddress() {
    var udataCur = $("#new-send-address").val()
    bridge.getAddressLabelAsync(udataCur);
}

function getAddressLabelResult(result) {
    console.log("getAddressLabelResult");
    var udataCur = result;
    var name;
    var g;
    var data;
    if (name = $("#new-send-label").val(), udataCur = $("#new-send-address").val(), g = result, "" !== g) {
      return $("#new-send-address-error").text('Error: address already in addressbook under "' + g + '"'), void $("#new-send-address").addClass("inputError");
    }
    var camelKey = 0;
    bridge.newAddressAsync(name, camelKey, udataCur, true);
}
function newAddressResult(result) {
    var udataCur = result;
    if (data = result, "" === data) {
      bridge.lastAddressError();
    } else {
      $("#add-address-modal").modal("hide");
    }
}

function lastAddressErrorResult(result) {
    var to = result;
    $("#new-send-address-error").text("Error: " + to);
    $("#new-send-address").addClass("inputError");
}

function addressBookInit() {
  var r20 = [{
    name : "Copy&nbsp;Address",
    fun : function() {
      copy("#addressbook .footable .selected .address");
    }
  }, {
    name : "Copy&nbsp;Public&nbsp;Key",
    fun : function() {
      copy("#addressbook .footable .selected .pubkey");
    }
  }, {
    name : "Copy&nbsp;Label",
    fun : function() {
      copy("#addressbook .footable .selected .label2");
    }
  }, {
    name : "Edit",
    fun : function() {
      $("#addressbook .footable .selected .label2.editable").dblclick();
    }
  }, {
    name : "Delete",
    fun : function() {
      var target = $("#addressbook .footable .selected .address");
        bridge.deleteAddress(target.text())
        target.closest("tr").remove();
    }
  }];
  $("#addressbook .footable tbody").on("contextmenu", function(ev) {
    $(ev.target).closest("tr").click();
  }).contextMenu(r20, {
    triggerOn : "contextmenu",
    sizeStyle : "content"
  });
  $("#filter-addressbook").on("input", function() {
    var $table = $("#addressbook-table");
    if ("" == $("#filter-addressbook").val()) {
      $table.data("footable-filter").clearFilter();
    }
    $("#addressbook-filter").val($("#filter-addressbook").val() + " " + $("#filter-addressbooktype").val());
    $table.trigger("footable_filter", {
      filter : $("#addressbook-filter").val()
    });
  });
  $("#filter-addressbooktype").change(function() {
    var $table = $("#addressbook-table");
    if ("" == $("#filter-addresstype").val()) {
      $table.data("footable-filter").clearFilter();
    }
    $("#addressbook-filter").val($("#filter-addressbook").val() + " " + $("#filter-addressbooktype").val());
    $table.trigger("footable_filter", {
      filter : $("#addressbook-filter").val()
    });
  });
}
function appendAddresses(err) {
  console.log("appending an address: " + err);
  if ("string" == typeof err) {
    if ("[]" == err) {
      return;
    }
    err = JSON.parse(err.replace(/,\]$/, "]"));
  }
  err.forEach(function(item) {
    var revisionCheckbox = $("#" + item.address);
    var target = "S" == item.type ? "#addressbook" : "#receive";
    if ("R" == item.type) {
      if (sendPage.initSendBalance(item)) {
        if (item.address.length < 75) {
            if (0 == revisionCheckbox.length) {
              $("#message-from-address").append("<option title='" + item.address + "' value='" + item.address + "'>" + item.label + "</option>");
            } else {
              $("#message-from-address option[value=" + item.address + "]").text(item.label);
            }
          }
      }
    }
    var param = "S" == item.type;
    var common = "n/a" !== item.pubkey;
    if (0 == revisionCheckbox.length) {
      $(target + " .footable tbody").append("<tr id='" + item.address + "' lbl='" + item.label + "'>                 <td style='padding-left:18px;' class='label2 editable' data-value='" + item.label_value + "'>" + item.label + "</td>                 <td class='address'>" + item.address + "</td>                 <td class='pubkey'>" + item.pubkey + "</td>                 <td class='addresstype'>" + (4 == item.at ? "Group" : 3 == item.at ? "BIP32" : 2 == item.at ? "Stealth" : "Normal") + "</td></tr>");
      $("#" + item.address).selection("tr").find(".editable").on("dblclick", function(event) {
        event.stopPropagation();
        updateValue($(this));
      }).attr("data-title", "Double click to edit").tooltip();
    } else {
      $("#" + item.address + " .label2").data("value", item.label_value).text(item.label);
      $("#" + item.address + " .pubkey").text(item.pubkey);
    }
  });
  $("#addressbook .footable,#receive .footable").trigger("footable_setup_paging");
}
function addressLookup(pair, dataAndEvents, coords) {
  function clear() {
    $("#address-lookup-filter").val($("#address-lookup-address-filter").val() + " " + $("#address-lookup-address-type").val());
    $table.trigger("footable_filter", {
      filter : $("#address-lookup-filter").val()
    });
  }
  var distanceToUserValue = $((dataAndEvents ? "#receive" : "#addressbook") + " table.footable > tbody").html();
  var $table = $("#address-lookup-table");
  $table.children("tbody").html(distanceToUserValue);
  $table.trigger("footable_initialize");
  $table.data("footable-filter").clearFilter();
  $("#address-lookup-table > tbody tr").selection().on("dblclick", function() {
    var values = pair.split(",");
    $("#" + values[0]).val($(this).attr("id").trim()).change();
    if (void 0 !== values[1]) {
      $("#" + values[1]).val($(this).attr("lbl").trim()).text($(this).attr("lbl").trim()).change();
    }
    $("#address-lookup-modal").modal("hide");
  });
  $("#address-lookup-address-filter").on("input", function() {
    if ("" == $("#lookup-address-filter").val()) {
      $table.data("footable-filter").clearFilter();
    }
    clear();
  });
  $("#address-lookup-address-type").change(function() {
    if ("" == $("#address-lookup-address-type").val()) {
      $table.data("footable-filter").clearFilter();
    }
    clear();
  });
  if (coords) {
    $("#address-lookup-address-type").val(coords);
    clear();
  }
}
function transactionPageInit() {
  var r20 = [{
    name : "Copy&nbsp;Amount",
    fun : function() {
      copy("#transactions .footable .selected .amount", "data-value");
    }
  }, {
    name : "Copy&nbsp;transaction&nbsp;ID",
    fun: function () {
        var trxId = $("#transactions .footable .selected").attr("id");
        copy(trxId.substring(0, trxId.length-4), "copy");
    }
  }, {
    name : "Edit&nbsp;label",
    fun : function() {
      $("#transactions .footable .selected .editable").dblclick();
    }
  }, {
    name : "Show&nbsp;transaction&nbsp;details",
    fun : function() {
      $("#transactions .footable .selected").dblclick();
    }
  }];
  $("#transactions .footable tbody").on("contextmenu", function(ev) {
    $(ev.target).closest("tr").click();
  }).contextMenu(r20, {
    triggerOn : "contextmenu",
    sizeStyle : "content"
  });
  $("#transactions .footable").on("footable_paging", function(data) {
    var self = filteredTransactions.slice(data.page * data.size);
    self = self.slice(0, data.size);
    var doc = $("#transactions .footable tbody");
    doc.html("");
    delete data.ft.pageInfo.pages[data.page];
    data.ft.pageInfo.pages[data.page] = self.map(function(result) {
      return result.html = formatTransaction(result), doc.append(result.html), $("#" + result.id)[0];
    });
    data.result = true;
    bindTransactionTableEvents();
  }).on("footable_create_pages", function(e) {
    var $table = $("#transactions .footable");
    if (!$($table.data("filter")).val()) {
      filteredTransactions = Transactions;
    }
    var i = $table.data("sorted");
    var err = 1 == $table.find("th.footable-sorted").length;
    var callback = "numeric";
    switch(i) {
      case 1:
        i = "d";
        break;
      case 2:
        i = "t_l";
        callback = "alpha";
        break;
      case 3:
        i = "ad";
        callback = "alpha";
        break;
      case 4:
        i = "n";
        callback = "alpha";
        break;
      case 5:
        i = "am";
        break;
      default:
        i = "c";
    }
    callback = e.ft.options.sorters[callback];
    filteredTransactions.sort(function(replies, rows) {
      return err ? callback(replies[i], rows[i]) : callback(rows[i], replies[i]);
    });
    delete e.ft.pageInfo.pages;
    e.ft.pageInfo.pages = [];
    var numRounds = Math.ceil(filteredTransactions.length / e.ft.pageInfo.pageSize);
    var copies = [];
    if (numRounds > 0) {
      var t = 0;
      for (;t < e.ft.pageInfo.pageSize;t++) {
        copies.push([]);
      }
      t = 0;
      for (;t < numRounds;t++) {
        e.ft.pageInfo.pages.push(copies);
      }
    }
  }).on("footable_filtering", function(options) {
    return!!options.clear || void(filteredTransactions = Transactions.filter(function(save) {
      var i;
      for (i in save) {
        if (save[i].toString().toLowerCase().indexOf(options.filter.toLowerCase()) !== -1) {
          return true;
        }
      }
      return false;
    }));
  });
}
function formatTransaction(o) {
  return "<tr id='" + o.id + "' data-title='" + o.tt + "'>                     <td class='trans-status' data-value='" + o.c + "'><center><i class='fa fa-lg " + o.s + "'></center></td>                    <td data-value='" + o.d + "'>" + o.d_s + "</td>                   <td class='trans_type'><img height='15' width='15' src='qrc:///assets/icons/tx_" + o.t + ".png' /> " + o.t_l + "</td>                    <td class='address' style='color:" + o.a_c + ";' data-value='" + o.ad + "' data-label='" + o.ad_l +
  "'><span class='editable'>" + o.ad_d + "</span></td>                    <td class='trans-nar'>" + o.n + "</td>                    <td class='amount' style='color:" + o.am_c + ";' data-value='" + o.am_d + "'>" + o.am_d + "</td>                 </tr>";
}
function visibleTransactions(checkSet) {
  if ("*" !== checkSet[0]) {
    Transactions = Transactions.filter(function(dataAndEvents) {
      return this.some(function(dataAndEvents) {
        return dataAndEvents == this;
      }, dataAndEvents.t_l);
    }, checkSet);
  }
}
function bindTransactionTableEvents() {
  $("#transactions .footable tbody tr").tooltip().on("click", function() {
    $(this).addClass("selected").siblings("tr").removeClass("selected");
  }).on("dblclick", function(dataAndEvents) {
    $(this).attr("href", "#transaction-info-modal");
    $("#transaction-info-modal").appendTo("body").modal("show");
      bridge.transactionDetails($(this).attr("id"));
    $(this).click();
    $(this).off("click");
    $(this).on("click", function() {
      $(this).addClass("selected").siblings("tr").removeClass("selected");
    });
  }).find(".editable").on("dblclick", function(event) {
    event.stopPropagation();
    event.preventDefault();
    updateValue($(this));
  }).attr("data-title", "Double click to edit").tooltip();
}
function appendTransactions(f) {
    console.log(f);
  if ("string" == typeof f) {
    if ("[]" == f) {
      return;
    }
    f = JSON.parse(f.replace(/,\]$/, "]"));
  }
  if (!(1 == f.length && f[0].id == -1)) {
    f.sort(function(a, b) {
      return a.d = parseInt(a.d), b.d = parseInt(b.d), b.d - a.d;
    });
    Transactions = Transactions.filter(function(deepDataAndEvents) {
      return 0 == this.some(function(finger) {
        return finger.id == this.id;
      }, deepDataAndEvents);
    }, f).concat(f);
    overviewPage.recent(f.slice(0, 7));
    $("#transactions .footable").trigger("footable_redraw");
  }
}
function getIconTitle(value) {
  return "unverified" == value ? "fa fa-cross " : "verified" == value ? "fa fa-check " : "contributor" == value ? "fa fa-cog " : "spectreteam" == value ? "fa fa-code " : "";
}

function signMessage() {
  $("#sign-signature").val("");
  var message;
  var callback;
  var msg;
  var length = "";
  message = $("#sign-address").val().trim();
  callback = $("#sign-message").val().trim();
    //TODO: SIGNAL bridge
  var result = bridge.signMessage(message, callback);
  return msg = result.error_msg, length = result.signed_signature, "" !== msg ? ($("#sign-result").removeClass("green"), $("#sign-result").addClass("red"), $("#sign-result").html(msg), false) : ($("#sign-signature").val(result.signed_signature), $("#sign-result").removeClass("red"), $("#sign-result").addClass("green"), $("#sign-result").html("Message signed successfully"), void 0);
}
function verifyMessage() {
  var message;
  var callback;
  var msg;
  var partials = "";
  message = $("#verify-address").val().trim();
  callback = $("#verify-message").val().trim();
  partials = $("#verify-signature").val().trim();
    //TODO: SIGNAL bridge
  var result = bridge.verifyMessage(message, callback, partials);
  return msg = result.error_msg, "" !== msg ? ($("#verify-result").removeClass("green"), $("#verify-result").addClass("red"), $("#verify-result").html(msg), false) : ($("#verify-result").removeClass("red"), $("#verify-result").addClass("green"), $("#verify-result").html("Message verified successfully"), void 0);
}
function iscrollReload(dataAndEvents) {
  messagesScroller.refresh();
  if (dataAndEvents === true) {
    messagesScroller.scrollTo(0, messagesScroller.maxScrollY, 0);
  }
}
function editorCommand(text, subs) {
  var startPos;
  var end;
  var _len;
  var y;
  var that = $("#message-text")[0];
  y = that.scrollTop;
  that.focus();
  if ("undefined" != typeof that.selectionStart) {
    startPos = that.selectionStart;
    end = that.selectionEnd;
    _len = text.length;
    if (subs) {
      text += that.value.substring(startPos, end) + subs;
    }
    that.value = that.value.substring(0, startPos) + text + that.value.substring(end, that.value.length);
    that.selectionStart = startPos + text.length - (subs ? subs.length : 0);
    that.selectionEnd = that.selectionStart;
  } else {
    that.value += text + subs;
  }
  that.scrollTop = y;
  that.focus();
}
function setupWizard(vid) {
  var collection = $("#" + vid + " > div");
  backbtnjs = '$("#key-options").show(); $("#wizards").hide();';
  fwdbtnjs = 'gotoWizard("new-key-wizard", 1);';
  $("#" + vid).prepend("<div id='backWiz' style='display:none;' class='btn btn-default btn-cons wizardback' onclick='" + backbtnjs + "' >Back</div>");
  $("#" + vid).prepend("<div id='fwdWiz'  style='display:none;' class='btn btn-default btn-cons wizardfwd'  onclick='" + fwdbtnjs + "' >Next Step</div>");
  collection.each(function(i) {
    $(this).addClass("step" + i);
    $(this).hide();
    $("#backWiz").hide();
  });
}
function gotoWizard(vid, node, dataAndEvents) {
  var collection = $("#wizards > div");
  if (validateJS = $("#" + vid + " .step" + (node - 1)).attr("validateJS"), dataAndEvents && void 0 !== validateJS) {
    var jsonObj = eval(validateJS);
    if (!jsonObj) {
      return false;
    }
  }
  collection.each(function(dataAndEvents) {
    $(this).hide();
  });
  var $allModules = $("#" + vid + " > div[class^=step]");
  var current = node;
  if (null == current) {
    current = 0;
  }
  if (0 == current) {
    $("#" + vid + " #backWiz").attr("onclick", '$(".wizardback").hide(); $("#wizards").show();');
    $("#" + vid + " #fwdWiz").attr("onclick", '$(".wizardback").hide(); gotoWizard("' + vid + '", 1, true);');
    $("#backWiz").hide();
  } else {
    $("#" + vid + " #backWiz").attr("onclick", 'gotoWizard("' + vid + '", ' + (current - 1) + " , false);");
    $("#" + vid + " #fwdWiz").attr("onclick", 'gotoWizard("' + vid + '", ' + (current + 1) + " , true);");
  }
  endWiz = $("#" + vid + " .step" + node).attr("endWiz");
  if (void 0 !== endWiz) {
    if ("" !== endWiz) {
      $("#" + vid + " #fwdWiz").attr("onclick", endWiz);
    }
  }
  $allModules.each(function(dataAndEvents) {
    $(this).hide();
  });
  $("#" + vid).show();
  stepJS = $("#" + vid + " .step" + current).attr("stepJS");
  if (dataAndEvents) {
    if (void 0 !== stepJS) {
      eval(stepJS);
    }
  }
  $("#" + vid + " .step" + current).fadeIn(0);
}
function dumpStrings() {
  function clone(dataAndEvents) {
    return'QT_TRANSLATE_NOOP("SpectreBridge", "' + dataAndEvents + '"),\n';
  }
  var data = "";
  $(".translate").each(function(dataAndEvents) {
    var d = clone($(this).text().trim());
    if (data.indexOf(d) == -1) {
      data += d;
    }
  });
  $("[data-title]").each(function(dataAndEvents) {
    var d = clone($(this).attr("data-title").trim());
    if (data.indexOf(d) == -1) {
      data += d;
    }
  });
  console.log(data);
}
//TODO: Update the translation function in a way that support QtWebEngine, QtWebEngine does not support calling a function that return results immediatly, it have to be done in a non blocking way using signals and slots
function translateStrings() {
//  $(".translate").each(function(dataAndEvents) {
//    var template = $(this).text();
//      console.log("template is " + template)
//      console.log("Translated template is " + bridge.translateHtmlString(template.trim()))
//    $(this).text(template.replace(template, bridge.translateHtmlString(template.trim())));
//  });
//  $("[data-title]").each(function(dataAndEvents) {
//    var title = $(this).attr("data-title");
//      console.log("Title is " + title)
//      console.log("Translated title is " + bridge.translateHtmlString(title.trim()))

//    $(this).attr("data-title", title.replace(title, bridge.translateHtmlString(title.trim())));
//  });
}
var breakpoint = 906;
var base58 = {
  base58Chars : "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz",
  check : function(target) {
    var el = $(target);
    var values = el.val();
    var i = 0;
    var valuesLen = values.length;
    for (;i < valuesLen;++i) {
      if (base58.base58Chars.indexOf(values[i]) == -1) {
        return el.css("background", "#1c5ce5").css("color", "white"), false;
      }
    }
    return el.css("background", "").css("color", ""), true;
  }
};
var pasteTo = "";
var unit = {
  type : 0,
  name : "XSPEC",
  display : "XSPEC",
  nameSpectre : "SPECTRE",
  displaySpectre : "SPECTRE",
  setType : function(v) {
    switch(this.type = void 0 == v ? 0 : v, v) {
      case 1:
        this.name = "mXSPEC";
        this.display = "mXSPEC";
        this.nameSpectre = "mSPECTRE";
        this.displaySpectre = "mSPECTRE";
        break;
      case 2:
        this.name = "uXSPEC";
        this.display = "&micro;XSPEC";
        this.nameSpectre = "uSPECTRE";
        this.displaySpectre = "&micro;SPECTRE";
        break;
      case 3:
        this.name = "sXSPEC";
        this.display = "xSpectoshi";
        this.nameSpectre = "sSPECTRE";
        this.displaySpectre = "Spectoshi";
        break;
      default:
        this.name = "XSPEC";
        this.display = "XSPEC";
        this.nameSpectre = "SPECTRE";
        this.displaySpectre = "SPECTRE";
    }
    $("td.unit,span.unit,div.unit").html(this.display);
    $("select.unit").val(v).trigger("change");
    $("input.unit").val(this.name);

    $("td.unitSpectre,span.unitSpectre,div.unitSpectre").html(this.displaySpectre);
    $("select.unitSpectre").val(v).trigger("change");
    $("input.unitSpectre").val(this.nameSpectre);
    overviewPage.updateBalance();
  },
  format : function(value, arg) {
    var results = $.isNumeric(value) ? null : $(value);
    switch(arg = void 0 == arg ? this.type : parseInt(arg), value = parseInt(void 0 == results ? value : void 0 == results.data("value") ? results.val() : results.data("value")), arg) {
      case 1:
        value /= 1E5;
        break;
      case 2:
        value /= 100;
        break;
      case 3:
        break;
      default:
        value /= 1E8;
    }
    return value = value.toFixed(this.mask(arg)), void 0 == results ? value : void results.val(value);
  },
  parse : function(value, text) {
    var results = $.isNumeric(value) ? null : $(value);
    text = void 0 == text ? this.type : parseInt(text);
    fp = void 0 == results ? value : results.val();
    if (void 0 == fp || fp.length < 1) {
      fp = ["0", "0"];
    } else {
      if ("." == fp[0]) {
        fp = ["0", fp.slice(1)];
      } else {
        fp = fp.split(".");
      }
    }
    value = parseInt(fp[0]);
    var n = this.mask(text);
    if (n > 0 && (value *= Math.pow(10, n)), n > 0 && fp.length > 1) {
      var bits = fp[1].split("");
      for (;bits.length > 1 && "0" == bits[bits.length - 1];) {
        bits.pop();
      }
      var increment = parseInt(bits.join(""));
      if (increment > 0) {
        n -= bits.length;
        if (n > 0) {
          increment *= Math.pow(10, n);
        }
        value += increment;
      }
    }
    return void 0 == results ? value : (results.data("value", value), void this.format(results, text));
  },
  mask : function(value) {
    switch(value = void 0 == value ? this.type : parseInt(value)) {
      case 1:
        return 5;
      case 2:
        return 2;
      case 3:
        return 0;
      default:
        return 8;
    }
  },
  keydown : function(e) {
    var key = e.which;
    var udataCur = $(e.target).siblings(".unit").val();
    if (190 == key || 110 == key) {
      return this.value.toString().indexOf(".") === -1 && 0 != unit.mask(udataCur) || e.preventDefault(), true;
    }
    if (e.shiftKey || !(key >= 96 && key <= 105 || key >= 48 && key <= 57)) {
      if (!(8 == key)) {
        if (!(9 == key)) {
          if (!(17 == key)) {
            if (!(46 == key)) {
              if (!(45 == key)) {
                if (!(key >= 35 && key <= 40)) {
                  if (!(e.ctrlKey && (65 == key || (67 == key || (86 == key || 88 == key))))) {
                    e.preventDefault();
                  }
                }
              }
            }
          }
        }
      }
    } else {
      var startPos = this.selectionStart;
      var endPos = this.value.indexOf(".");
      if ("Range" !== document.getSelection().type && (startPos > endPos && (this.value.indexOf(".") !== -1 && this.value.length - 1 - endPos >= unit.mask(udataCur)))) {
        if ("0" == this.value[this.value.length - 1] && startPos < this.value.length) {
          return this.value = this.value.slice(0, -1), this.selectionStart = startPos, void(this.selectionEnd = startPos);
        }
        e.preventDefault();
      }
    }
  },
  paste : function(e) {
    var subStr = e.originalEvent.clipboardData.getData("text/plain");
    if (!$.isNumeric(subStr) || this.value.indexOf(".") !== -1 && "Range" !== document.getSelection().type) {
      e.preventDefault();
    }
  }
};
var contextMenus = [];
var overviewPage = {
  init : function() {
    this.balance = $(".balance");
    this.spectreBal = $(".spectre_balance");
    this.reserved = $("#reserved");
    this.stake = $("#stake");
    this.unconfirmed = $("#unconfirmed");
    this.immature = $("#immature");
    this.total = $("#total");
  },
  updateBalance : function(ns, key, id, name, type) {
    if (void 0 == ns) {
      ns = this.balance.data("orig");
      key = this.spectreBal.data("orig");
      id = this.stake.data("orig");
      name = this.unconfirmed.data("orig");
      type = this.immature.data("orig");
    } else {
      this.balance.data("orig", ns);
      this.spectreBal.data("orig", key);
      this.stake.data("orig", id);
      this.unconfirmed.data("orig", name);
      this.immature.data("orig", type);
    }
    this.formatValue("balance", ns);
    this.formatValue("spectreBal", key);
    this.formatValue("stake", id);
    this.formatValue("unconfirmed", name);
    this.formatValue("immature", type);
    this.formatValue("total", ns + id + name + type + key);
  },
  updateReserved : function(name) {
    this.formatValue("reserved", name);
  },
  formatValue : function(target, name) {
    if ("total" === target && (void 0 !== name && !isNaN(name))) {
      var data = unit.format(name).split(".");
      $("#total-big > span:first-child").text(data[0]);
      if (unit.type == 3) {
        $("#total-big .light-red").toggle(false);
        $("#total-big .cents").toggle(false);
      }
      else {
        $("#total-big .cents").text(data[1]);
        $("#total-big .light-red").toggle(true);
        $("#total-big .cents").toggle(true);
      }
    }
    if ("stake" === target && (void 0 !== name && !isNaN(name))) {
      if (0 == name) {
        $("#staking-big").addClass("not-staking");
      } else {
        $("#staking-big").removeClass("not-staking");
      }
      data = unit.format(name).split(".");
      $("#staking-big > span:first-child").text(data[0]);
      $("#staking-big .cents").text(data[1]);
    }
    var targetHTML = this[target];
    if (0 == name) {
      targetHTML.html("");
      targetHTML.parent("tr").hide();
    } else {
      if ("balance" === target) {
        targetHTML.text(unit.format(name) + " " + unit.display);
      }
      else if ("spectreBal" === target) {
        targetHTML.text(unit.format(name) + " " + unit.displaySpectre);
      }
      else {
        targetHTML.text(unit.format(name));
      }
      targetHTML.parent("tr").show();
    }
  },
  recent : function(codeSegments) {
    var i = 0;
    for (;i < codeSegments.length;i++) {
      overviewPage.updateTransaction(codeSegments[i]);
    }
    $("#recenttxns [data-title]").tooltip();
  },
  updateTransaction : function(message) {
    var update = function(data) {
      return "<tr><td class='text-left' width='30%' style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><center><label style='margin-top:6px;' class='label label-important inline fs-12'>" + ("input" == data.t ? "Received" : "output" == data.t ? "Sent" : "inout" == data.t ? "In-Out" : "Stake") + "</label></center></td><td class='text-left' style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><center><a id='" + data.id.substring(data.id.length-20) + "' data-title='" + data.tt +
      "' href='#' onclick='$(\"#navitems [href=#transactions]\").click();$(\"#" + data.id + "\").click();'> " + unit.format(data.am) + " " + ((data.am_curr === 'SPECTRE') ? unit.displaySpectre : unit.display) + " </a></center></td><td width='30%' style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><span class='overview_date' data-value='" + data.d + "'><center>" + data.d_s + "</center></span></td></tr>";
    };
    var idfirst = message.id.substring(message.id.length-20);
    if (0 == $("#" + idfirst).attr("data-title", message.tt).length) {
      var $items = $("#recenttxns tr");
      var error = update(message);
      var o = false;
      var i = 0;
      for (;i < $items.length;i++) {
        var $this = $($items[i]);
        if (parseInt(message.d) > parseInt($this.find(".overview_date").data("value"))) {
          $this.before(error);
          o = true;
          break;
        }
      }
      if (!o) {
        $("#recenttxns").append(error);
      }
      $items = $("#recenttxns tr");
      for (;$items.length > 8;) {
        $("#recenttxns tr:last").remove();
        $items = $("#recenttxns tr");
      }
    }
  },
  clientInfo : function() {
    $("#version").text(bridge.info.build.replace(/\-[\w\d]*$/, ""));
    $("#clientinfo").attr("data-title", "Build Desc: " + bridge.info.build + "\nBuild Date: " + bridge.info.date).tooltip();
  },
  encryptionStatusChanged : function(dataAndEvents) {
    switch(dataAndEvents) {
      case 0:
      ;
      case 1:
      ;
      case 2:
      ;
    }
  }
};
var optionsPage = {
  init : function() {
  },
  connectSignals : function() {
      bridge.getOptionResult.connect(this.getOptionResult);
  },
  getOptionResult : function (result) {
      function update($el) {
        $el = $(this);
        var val = $el.prop("checked");
        var codeSegments = $el.data("linked");
        if (codeSegments) {
          codeSegments = codeSegments.split(" ");
          var i = 0;
          for (;i < codeSegments.length;i++) {
            var $wrapper = $("#" + codeSegments[i] + ",[for=" + codeSegments[i] + "]").attr("disabled", !val);
            if (val) {
              $wrapper.removeClass("disabled");
            } else {
              $wrapper.addClass("disabled");
            }
          }
        }
      }
      var list = result;
      console.log('result');
      console.log(result);
      $("#options-ok,#options-apply").addClass("disabled");
      var name;
      for (name in list) {
        var el = $("#opt" + name);
        var value = list[name];
        var data = list["opt" + name];
        if (0 != el.length) {
          if (data) {
            el.html("");
            var type;
            for (type in data) {
              if ("string" == typeof type && ($.isArray(data[type]) && !$.isNumeric(type))) {
                el.append("<optgroup label='" + type[0].toUpperCase() + type.slice(1) + "'>");
                var x = 0;
                for (;x < data[type].length;x++) {
                  el.append("<option>" + data[type][x] + "</option>");
                }
              } else {
                el.append("<option" + ($.isNumeric(type) ? "" : " value='" + type + "'") + ">" + data[type] + "</option>");
              }
            }
          }
          if (el.is(":checkbox")) {
            el.prop("checked", value === true || "true" === value);
            el.off("change");
            el.on("change", update);
            el.change();
          } else {
            if (el.is("select[multiple]") && "*" === value) {
              el.find("option").attr("selected", true);
            } else {
              el.val(value);
            }
          }
          el.one("change", function() {
            $("#options-ok,#options-apply").removeClass("disabled");
          });
        } else {
          if (name.indexOf("opt") == -1) {
            console.log("Option element not available for %s", name);
          }
        }
      }
  },
  update : function() {
      bridge.getOptions();
  },
  save : function() {
    var o = bridge.info.options;
    var cache = {};
    var prop;
    for (prop in o) {
      var $field = $("#opt" + prop);
      var n = o[prop];
      var value = false;
      if (!(null != n && "false" != n)) {
        n = false;
      }
      if (0 != $field.length) {
        if ($field.is(":checkbox")) {
          value = $field.prop("checked");
        } else {
          if ($field.is("select[multiple]")) {
            value = $field.val();
            if (null === value) {
              value = "*";
            }
          } else {
            value = $field.val();
          }
        }
        if (n != value) {
          if (n.toString() !== value.toString()) {
            cache[prop] = value;
          }
        }
      }
    }
    if (!$.isEmptyObject(cache)) {
        console.log("calling bridge.userAction.optionsChanged")
      bridge.userAction({
        optionsChanged : cache
      });
      optionsPage.update();
      if (cache.hasOwnProperty("AutoRingSize")) {
        changeTxnType();
      }
    } else {
        console.log("options cache is empty")
    }
  }
};
var Transactions = [];
var filteredTransactions = [];
var current_key = "";

var chainDataPage = {
  anonOutputs : {},
    connectSignals: function() {
        console.log("chainDataPage.connectSignals");
        bridge.listAnonOutputsResult.connect(this.listAnonOutputsResult);
    },
  init : function() {
    $("#show-own-outputs,#show-all-outputs").on("click", function(ev) {
      $(ev.target).hide().siblings("a").show();
    });
    $("#show-own-outputs").on("click", function() {
      $("#chaindata .footable tbody tr>td:first-child+td").each(function() {
        if (0 == $(this).text()) {
          $(this).parents("tr").hide();
        }
      });
    });
    $("#show-all-outputs").on("click", function() {
      $("#chaindata .footable tbody tr:hidden").show();
    });
  },
  updateAnonOutputs : function() {
      bridge.listAnonOutputs();
  },
  listAnonOutputsResult : function(result) {
      chainDataPage.anonOutputs = result;
      var tagList = $("#chaindata .footable tbody");
      tagList.html("");
      for (value in chainDataPage.anonOutputs) {
        var state = chainDataPage.anonOutputs[value];
        tagList.append("<tr>                    <td data-value=" + value + ">" + state.value_s + "</td>                    <td>" + state.owned_outputs + (state.owned_outputs == state.owned_mature ? "" : " (<b>" + state.owned_mature + "</b>)") + "</td>                    <td>" + state.system_outputs + " (" + state.system_mature + ")</td>                    <td>" + state.system_spends + "</td>                    <td>" + state.least_depth + "</td>                </tr>");
      }
      $("#chaindata .footable").trigger("footable_initialize");
  }
};
var blockExplorerPage = {
  blockHeader : {},

    connectSignals: function() {
        console.log("blockExplorerPage.connectSignals");
        bridge.findBlockResult.connect(this.findBlockResult);
        bridge.listLatestBlocksResult.connect(this.listLatestBlocksResult);
        bridge.listTransactionsForBlockResult.connect(this.listTransactionsForBlockResult);
        bridge.txnDetailsResult.connect(this.txnDetailsResult);
        bridge.blockDetailsResult.connect(this.blockDetailsResult);
    },

  findBlock : function(value) {
      console.log("findBlock : function(value)");
      console.log(value);

    if ("" === value || null === value) {
      blockExplorerPage.updateLatestBlocks();
    } else {
        bridge.findBlock(value);
        //will callback findBlockResult
    }
  },

    findBlockResult : function(result) {
        console.log("findBlockResult : function(result)");
        blockExplorerPage.foundBlock = result;
        if (blockExplorerPage.foundBlock, "" !== blockExplorerPage.foundBlock.error_msg) {
          return $("#latest-blocks-table  > tbody").html(""), $("#block-txs-table > tbody").html(""), $("#block-txs-table").addClass("none"), alert(blockExplorerPage.foundBlock.error_msg), false;
        }
        var tagList = $("#latest-blocks-table  > tbody");
        tagList.html("");
        var $title = $("#block-txs-table  > tbody");
        $title.html("");
        $("#block-txs-table").addClass("none");
        tagList.append("<tr data-value=" + blockExplorerPage.foundBlock.block_hash + ">                                     <td>" + blockExplorerPage.foundBlock.block_hash + "</td>                                     <td>" + blockExplorerPage.foundBlock.block_height + "</td>                                     <td>" + blockExplorerPage.foundBlock.block_timestamp + "</td>                                     <td>" + blockExplorerPage.foundBlock.block_transactions + "</td>                        </tr>");
        blockExplorerPage.prepareBlockTable();
    },

  updateLatestBlocks : function() {
    blockExplorerPage.latestBlocks = bridge.listLatestBlocks();
  },

    listLatestBlocksResult : function(result) {
        console.log("function listLatestBlocksResult")
        console.log(result)
        blockExplorerPage.latestBlocks = result;
        var $title = $("#block-txs-table  > tbody");
        $title.html("");
        $("#block-txs-table").addClass("none");
        var tagList = $("#latest-blocks-table  > tbody");
        tagList.html("");
        for (value in blockExplorerPage.latestBlocks) {
          var cachedObj = blockExplorerPage.latestBlocks[value];
          tagList.append("<tr data-value=" + cachedObj.block_hash + ">                         <td>" + cachedObj.block_hash + "</td>                         <td>" + cachedObj.block_height + "</td>                         <td>" + cachedObj.block_timestamp + "</td>                         <td>" + cachedObj.block_transactions + "</td>                         </tr>");
        }
        blockExplorerPage.prepareBlockTable();
    },

  prepareBlockTable : function() {
    $("#latest-blocks-table  > tbody tr").selection().on("click", function() {
      var r20 = $(this).attr("data-value").trim();
      blockExplorerPage.blkTxns = bridge.listTransactionsForBlock(r20);
    }).on("dblclick", function(dataAndEvents) {
      $("#block-info-modal").appendTo("body").modal("show");
      selectedBlock = bridge.blockDetails($(this).attr("data-value").trim());
    }).find(".editable");
  },

     listTransactionsForBlockResult : function(blkHash, result) {
        blockExplorerPage.blkTxns = result;
        var tagList = $("#block-txs-table  > tbody");
        tagList.html("");
        for (value in blockExplorerPage.blkTxns) {
          var cachedObj = blockExplorerPage.blkTxns[value];
          tagList.append("<tr data-value=" + cachedObj.transaction_hash + ">                                    <td>" + cachedObj.transaction_hash + "</td>                                    <td>" + cachedObj.transaction_value + "</td>                                    </tr>");
        }
        $("#block-txs-table").removeClass("none");
        $("#block-txs-table > tbody tr").selection().on("dblclick", function(dataAndEvents) {
          $("#blkexp-txn-modal").appendTo("body").modal("show");
          selectedTxn = bridge.txnDetails(blkHash, $(this).attr("data-value").trim());
        }).find(".editable");
    },

    txnDetailsResult : function(result) {
        selectedTxn = result;
        if ("" == selectedTxn.error_msg) {
          $("#txn-hash").html(selectedTxn.transaction_hash);
          $("#txn-size").html(selectedTxn.transaction_size);
          $("#txn-rcvtime").html(selectedTxn.transaction_rcv_time);
          $("#txn-minetime").html(selectedTxn.transaction_mined_time);
          $("#txn-blkhash").html(selectedTxn.transaction_block_hash);
          $("#txn-reward").html(selectedTxn.transaction_reward);
          $("#txn-confirmations").html(selectedTxn.transaction_confirmations);
          $("#txn-value").html(selectedTxn.transaction_value);
          $("#error-msg").html(selectedTxn.error_msg);
          if (selectedTxn.transaction_reward > 0) {
            $("#lbl-reward-or-fee").html("<strong>Reward</strong>");
            $("#txn-reward").html(selectedTxn.transaction_reward);
          } else {
            $("#lbl-reward-or-fee").html("<strong>Fee</strong>");
            $("#txn-reward").html(selectedTxn.transaction_reward * -1);
          }
        }
        var tagList = $("#txn-detail-inputs > tbody");
        tagList.html("");
        for (value in selectedTxn.transaction_inputs) {
          var cachedObj = selectedTxn.transaction_inputs[value];
          tagList.append("<tr data-value=" + cachedObj.input_source_address + ">                                                   <td>" + cachedObj.input_source_address + "</td>                                                   <td style='text-align:right'>" + cachedObj.input_value + "</td>                                                </tr>");
        }
        var $options = $("#txn-detail-outputs > tbody");
        $options.html("");
        for (value in selectedTxn.transaction_outputs) {
          var style = selectedTxn.transaction_outputs[value];
          $options.append("<tr data-value=" + style.output_source_address + ">                                                 <td>" + style.output_source_address + "</td>                                                 <td style='text-align:right'>" + style.output_value + "</td>                                            </tr>");
        }
    },

    blockDetailsResult : function(result) {
        selectedBlock = result;
        if (selectedBlock) {
          $("#blk-hash").html(selectedBlock.block_hash);
          $("#blk-numtx").html(selectedBlock.block_transactions);
          $("#blk-height").html(selectedBlock.block_height);
          $("#blk-type").html(selectedBlock.block_type);
          $("#blk-reward").html(selectedBlock.block_reward);
          $("#blk-timestamp").html(selectedBlock.block_timestamp);
          $("#blk-merkleroot").html(selectedBlock.block_merkle_root);
          $("#blk-prevblock").html(selectedBlock.block_prev_block);
          $("#blk-nextblock").html(selectedBlock.block_next_block);
          $("#blk-difficulty").html(selectedBlock.block_difficulty);
          $("#blk-bits").html(selectedBlock.block_bits);
          $("#blk-size").html(selectedBlock.block_size);
          $("#blk-version").html(selectedBlock.block_version);
          $("#blk-nonce").html(selectedBlock.block_nonce);
        }
        $(this).click().off("click").selection();
    }

};
var walletManagementPage = {

    connectSignals : function() {
        console.log("walletManagementPage.connectSignals");
        bridge.importFromMnemonicResult.connect(this.importFromMnemonicResult);
        bridge.getNewMnemonicResult.connect(this.getNewMnemonicResult);
        bridge.extKeyAccListResult.connect(this.extKeyAccListResult);
        bridge.extKeyListResult.connect(this.extKeyListResult);
//        bridge.extKeyImportResult.connect(this.extKeyImportResult);
        bridge.extKeySetDefaultResult.connect(this.extKeySetDefaultResult);
        bridge.extKeySetMasterResult.connect(this.extKeySetMasterResult);
        bridge.extKeySetActiveResult.connect(this.extKeySetActiveResult);
    },

  init : function() {
    setupWizard("new-key-wizard");
    setupWizard("recover-key-wizard");
    setupWizard("open-key-wizard");
  },
  newMnemonic : function() {
    bridge.getNewMnemonic($("#new-account-passphrase").val(), $("#new-account-language").val());
  },
    getNewMnemonicResult : function(result) {
        var c = result;
        var g = c.error_msg;
        var i = c.mnemonic;
        if ("" !== g) {
          alert(g);
        } else {
          $("#new-key-mnemonic").val(i);
        }
    },
  compareMnemonics : function() {
    var i = $("#new-key-mnemonic").val().trim();
    var last = $("#validate-key-mnemonic").val().trim();
    return i == last ? ($("#validate-key-mnemonic").removeClass("red"), $("#validate-key-mnemonic").val(""), true) : ($("#validate-key-mnemonic").addClass("red"), alert("The recovery phrase you provided does not match the recovery phrase that was generated earlier - please go back and check to make sure you have copied it down correctly."), false);
  },
  gotoPage : function(page) {
    $("#navitems a[href='#" + page + "']").trigger("click");
  },
  prepareAccountTable : function() {
    $("#extkey-account-table  > tbody tr").selection().on("click", function() {
      var $this = $("#extkey-table > tbody > tr");
      $this.removeClass("selected");
    });
  },
  updateAccountList : function() {
    bridge.extKeyAccList();
  },
  extKeyAccListResult : function(result) {
      walletManagementPage.accountList = result;
      var tagList = $("#extkey-account-table  > tbody");
      tagList.html("");
      for (value in walletManagementPage.accountList) {
        var result = walletManagementPage.accountList[value];
        tagList.append("<tr data-value=" + result.id + " active-flag=" + result.active + ">                         <td>" + result.id + "</td>                         <td>" + result.label + "</td>                         <td>" + result.created_at + '</td>                         <td class="center-margin"><i style="font-size: 1.2em; margin: auto;" ' + ("true" == result.active ? 'class="fa fa-circle green-circle"' : 'class="fa fa-circle red-circle"') + ' ></i></td>                         <td style="font-size: 1em; margin-bottom: 6px;">' +
        (void 0 !== result.default_account ? "<i class='center fa fa-check'></i>" : "") + "</td>                         </tr>");
      }
      walletManagementPage.prepareAccountTable();
  },
  prepareKeyTable : function() {
    $("#extkey-table  > tbody tr").selection().on("click", function() {
      var $this = $("#extkey-account-table > tbody > tr");
      $this.removeClass("selected");
    });
  },
  updateKeyList : function() {
      bridge.extKeyList();
  },
  extKeyListResult : function(result) {
      walletManagementPage.keyList = result;
      var tagList = $("#extkey-table  > tbody");
      tagList.html("");
      for (value in walletManagementPage.keyList) {
        var node = walletManagementPage.keyList[value];
        tagList.append("<tr data-value=" + node.id + " active-flag=" + node.active + ">                         <td>" + node.id + "</td>                         <td>" + node.label + "</td>                         <td>" + node.path + '</td>                         <td><i style="font-size: 1.2em; margin: auto;" ' + ("true" == node.active ? 'class="fa fa-circle green-circle"' : 'class="fa fa-circle red-circle"') + ' ></i></td>                         <td style="font-size: 1em; margin-bottom: 6px;">' +
        (void 0 !== node.current_master ? "<i class='center fa fa-check'></i>" : "") + "</td>                         </tr>");
      }
      walletManagementPage.prepareKeyTable();
  },
  newKey : function() {
    bridge.importFromMnemonic($("#new-key-mnemonic").val().trim(), $("#new-account-passphrase").val().trim(), $("#new-account-label").val().trim(), $("#new-account-bip44").prop("checked"), 0);
  },
  importFromMnemonicResult : function(result) {
      if (result, "" !== result.error_msg) {
        return alert(result.error_msg), false;
      }
  },
  recoverKey : function() {
    bridge.importFromMnemonic($("#recover-key-mnemonic").val().trim(), $("#recover-passphrase").val().trim(), $("#recover-account-label").val().trim(), $("#recover-bip44").prop("checked"), 1443657600);
  },

  setMaster : function() {
    var revisionCheckbox = $("#extkey-table tr.selected");
    return revisionCheckbox.length ? (selected = $("#extkey-table tr.selected").attr("data-value").trim(), void 0 === selected || "" === selected ? (alert("Select a key from the table to set a Master."), false) : (result = bridge.extKeySetMaster(selected), "" !== result.error_msg ? (alert(result.error_msg), false) : void walletManagementPage.updateKeyList())) : (alert("Please select a key to set it as master."), false);
  },
  extKeySetMasterResult : function(result) {

  },
  setDefault : function() {
    var revisionCheckbox = $("#extkey-account-table tr.selected");
    return revisionCheckbox.length ? (selected = $("#extkey-account-table tr.selected").attr("data-value").trim(), void 0 === selected || "" === selected ? (alert("Select an account from the table to set a default."), false) : (result = bridge.extKeySetDefault(selected), "" !== result.error_msg ? (alert(result.error_msg), false) : void walletManagementPage.updateAccountList())) : (alert("Please select an account to set it as default."), false);
  },
  extKeySetDefaultResult : function(result) {

  },
  changeActiveFlag : function() {
    var e = false;
    var add = $("#extkey-account-table tr.selected");
    var $target = $("#extkey-table tr.selected");
    return add.length || $target.length ? (add.length ? (selected = add.attr("data-value").trim(), active = add.attr("active-flag").trim(), e = true) : (selected = $target.attr("data-value").trim(), active = $target.attr("active-flag").trim()), void 0 === selected || "" === selected ? (alert("Please select an account or key to change the active status."), false) : (result = bridge.extKeySetActive(selected, active), "" !== result.error_msg ? (alert(result.error_msg), false) : void(e ? walletManagementPage.updateAccountList() :
    walletManagementPage.updateKeyList()))) : (alert("Please select an account or key to change the active status."), false);
  },
  extKeySetActiveResult : function(result) {

  }
};

window.onload = function() {
  overviewPage.init();
  receivePageInit();
  transactionPageInit();
  addressBookInit();
  chainDataPage.init();
  walletManagementPage.init();

  $(".footable,.footable-lookup").footable({
    breakpoints : {
      phone : 480,
      tablet : 700
    },
    delay : 50
  }).on({
    footable_breakpoint : function() {
    },
    footable_row_expanded : function(dataAndEvents) {
      var el = $(this).find(".editable");
      el.off("dblclick").on("dblclick", function(event) {
        event.stopPropagation();
        updateValue($(this));
      }).attr("data-title", "Double click to edit").tooltip();
    }
  });
  $(".editable").on("dblclick", function(event) {
    event.stopPropagation();
    updateValue($(this));
  }).attr("data-title", "Double click to edit %column%");
  window.onresize = function(event) {
    if (window.innerWidth > breakpoint) {
      $("#layout").removeClass("active");
    }
  };
  if (bridge) {
    $("[href='#about']").on("click", function() {
      bridge.userAction(["aboutClicked"]);
    });
  }
  $(".footable > tbody tr").selection();

  connectSignals();
};

$.fn.selection = function(element) {
  return element || (element = "tr"), this.on("click", function() {
    $(this).addClass("selected").siblings(element).removeClass("selected");
  });
};
