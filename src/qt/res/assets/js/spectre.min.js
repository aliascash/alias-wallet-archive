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
  if ("Group" === selected) {
    name.replace("group_", "");
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
function updateValueChat(values, index) {
  var isFunction = values.data("value");
  var c = contacts[index];
  return void 0 != c && (values.html('<input class="new_chat_value" type="text" onchange="bridge.updateAddressLabel(\'' + c.address + '\', this.value);" value="' + isFunction + '" size=35 style="display:inline;" />'), $("#chat-header .new_chat_value").focus(), $("#chat-header .new_chat_value").on("contextmenu", function(event) {
    event.stopPropagation();
  }), $("#chat-header .new_chat_value").keypress(function(event) {
    if (13 == event.which) {
      event.preventDefault();
      var $radio = $("#chat-header .new_chat_value");
      if (void 0 == $radio || void 0 === $radio.val()) {
        return false;
      }
      var value = $radio.val().trim();
      if (void 0 == value) {
        return false;
      }
      if (0 === value.length) {
        return false;
      }
      values.html(value);
      contacts[current_key].label = value;
      $("#chat-header").data("value", value);
      $("#contact-" + current_key + " .contact-info .contact-name").text(value);
      $("#contact-book-" + current_key + " .contact-info .contact-name").text(value);
    }
  }), $("#chat-header .new_chat_value").click(function(event) {
    event.stopPropagation();
  }), void $(document).one("click", function() {
    var $radio = $("#chat-header .new_chat_value");
    if (void 0 === typeof $radio || void 0 === $radio.val()) {
      return false;
    }
    var value = $radio.val().trim();
    return void 0 != value && (values.html(value), contacts[current_key].label = value, $("#chat-header").data("value", value), $("#contact-" + current_key + " .contact-info .contact-name").text(value), void $("#contact-book-" + current_key + " .contact-info .contact-name").text(value));
  }));
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
  bridge.emitMessages.connect(appendMessages);
  bridge.emitMessage.connect(appendMessage);
  bridge.emitCoinControlUpdate.connect(updateCoinControlInfo);
  bridge.triggerElement.connect(triggerElement);
  bridge.emitReceipient.connect(addRecipientDetail);
  bridge.networkAlert.connect(networkAlert);
  bridge.getAddressLabelResult.connect(getAddressLabelResult);
  bridge.newAddressResult.connect(newAddressResult);
  bridge.lastAddressErrorResult.connect(lastAddressErrorResult);
  bridge.createGroupChatResult.connect(createGroupChatResult);
  bridge.getAddressLabelForSelectorResult.connect(getAddressLabelForSelectorResult);

  blockExplorerPage.connectSignals();
  walletManagementPage.connectSignals();
  optionsPage.connectSignals();
  chainDataPage.connectSignals();


  bridge.validateAddressResult.connect(validateAddressResult);
  bridge.addRecipientResult.connect(addRecipientResult);
  bridge.sendCoinsResult.connect(sendCoinsResult);
  bridge.transactionDetailsResult.connect(transactionDetailsResult);
  bridge.sendMessageResult.connect(sendMessageResult);
  bridge.joinGroupChatResult.connect(joinGroupChatResult);


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
  var r20 = "4" == throughArgs ? "group_" + $("#new-address-label").val() : $("#new-address-label").val();
  newAdd = bridge.newAddress(r20, throughArgs, '', false);
}

function newAddressResult(result) {
    console.log('newAddressResult');
    newAdd = result;
    $("#add-address-modal").modal("hide");
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
      updateContact(name, current_key, udataCur, false);
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
  contact_book_list = $("#contact-book-list ul");
  err.forEach(function(item) {
    var revisionCheckbox = $("#" + item.address);
    var target = "S" == item.type ? "#addressbook" : 0 !== item.label.lastIndexOf("group_", 0) ? "#receive" : "#addressbook";
    var $src = $("#invite-modal-" + item.address);
    var $slide = $("#invite-modal-" + item.address);
    if ("R" == item.type) {
      if (sendPage.initSendBalance(item)) {
        if (item.address.length < 75) {
          if (0 !== item.label.lastIndexOf("group_", 0)) {
            if (0 == revisionCheckbox.length) {
              $("#message-from-address").append("<option title='" + item.address + "' value='" + item.address + "'>" + item.label + "</option>");
            } else {
              $("#message-from-address option[value=" + item.address + "]").text(item.label);
            }
            if (initialAddress) {
              $("#message-from-address").prepend("<option title='Anonymous' value='anon' selected>Anonymous</option>");
              $(".user-name").text(Name);
              $(".user-address").text(item.address);
              initialAddress = false;
            }
          }
        }
      }
    }
    var recurring = 4 == item.at || 0 === item.label.lastIndexOf("group_", 0);
    var param = "S" == item.type;
    if (recurring) {
      item.at = 4;
      item.label = item.label.replace("group_", "");
      item.label_value = item.label_value.replace("group_", "");
      param = true;
    }
    var common = "n/a" !== item.pubkey;
    if (param && (common && (createContact(item.label, item.address, recurring, true), appendContact(item.address, false, true))), !recurring && (param && common)) {
      if (0 == $src.length) {
        var lineSeparator = "<tr id='invite-modal-" + item.address + "' lbl='" + item.label + "'>                   <td style='padding-left:18px;' class='label2' data-value='" + item.label_value + "'>" + item.label + "</td>                   <td class='address'>" + item.address + "</td>                   <td class='invite footable-visible footable-last-column'><input type='checkbox' class='checkbox'></input></td>                   </tr>";
        $("#invite-modal-tbody").append(lineSeparator);
      } else {
        $("#invite-modal-" + item.address + " .label2").text(item.label);
      }
      if (0 == $slide.length) {
        lineSeparator = "<tr  id='group-modal-" + item.address + "' lbl='" + item.label + "'>                   <td style='padding-left:18px;' class='label2' data-value='" + item.label_value + "'>" + item.label + "</td>                   <td class='address'>" + item.address + "</td>                   <td class='invite footable-visible footable-last-column'><input type='checkbox' class='checkbox'></input></td>                   </tr>";
        $("#group-modal-tbody").append(lineSeparator);
      } else {
        $("#group-modal-" + item.address + " .label2").text(item.label);
      }
    }
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
      case 0:
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
  return "<tr id='" + o.id + "' data-title='" + o.tt + "'>                    <td data-value='" + o.d + "'>" + o.d_s + "</td>                    <td class='trans-status' data-value='" + o.c + "'><center><i class='fa fa-lg " + o.s + "'></center></td>                    <td class='trans_type'><img height='15' width='15' src='qrc:///assets/icons/tx_" + o.t + ".png' /> " + o.t_l + "</td>                    <td class='address' style='color:" + o.a_c + ";' data-value='" + o.ad + "' data-label='" + o.ad_l +
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
function spectreChatInit() {
  var r20 = [{
    name : "Send&nbsp;Spectrecoin",
    fun : function() {
      clearRecipients();
      $("#pay_to0").val($("#contact-list .selected .contact-address").text());
      $("#navpanel [href=#send]").click();
    }
  }, {
    name : "Copy&nbsp;Address",
    fun : function() {
      copy("#contact-list .selected .contact-address");
    }
  }, {
    name : "Private&nbsp;Message",
    fun : function() {
      $("#message-text").focus();
    }
  }];
  $("#contact-list").on("contextmenu", function(ev) {
    $(ev.target).closest("li").click();
  }).contextMenu(r20, {
    triggerOn : "contextmenu",
    sizeStyle : "content"
  });
  r20 = [{
    name : "Copy&nbsp;Selected",
    fun : function() {
      var textfield = $("#message-text")[0];
      if ("undefined" != typeof textfield.selectionStart) {
        copy(textfield.value.substring(textfield.selectionStart, textfield.selectionEnd), "copy");
      }
    }
  }, {
    name : "Paste",
    fun : function() {
      paste("#pasteTo");
      var textfield = $("#message-text")[0];
      if ("undefined" != typeof textfield.selectionStart) {
        textfield.value = textfield.value.substring(textfield.selectionStart, 0) + $("#pasteTo").val() + textfield.value.substring(textfield.selectionStart);
      } else {
        textfield.value += $("#pasteTo").val();
      }
    }
  }];
  $("#message-text").contextMenu(r20, {
    triggerOn : "contextmenu",
    sizeStyle : "content"
  });
  $("#message-text").keypress(function(event) {
    if (13 == event.which && !event.shiftKey) {
      if (event.preventDefault(), "" == $("#message-text").val()) {
        return 0;
      }
      removeNotificationCount();
      sendMessage();
    }
  });
  $("#messages").selection().on("click", function(dataAndEvents) {
    if ("" !== current_key) {
      removeNotificationCount(current_key);
    }
  });
  $("#contact-list").on("mouseover", function() {
    contactScroll.refresh();
  });
  $("#contact-group-list").on("mouseover", function() {
    contactGroupScroll.refresh();
  });
  $("#contact-book-list").on("mouseover", function() {
    contactBookScroll.refresh();
  });
}
function appendMessages(qs, deepDataAndEvents) {
  if (contact_list = $("#contact-list ul"), contact_group_list = $("#contact-group-list ul"), deepDataAndEvents) {
    var index;
    for (index in contacts) {
      if (contacts[index].messages.length > 0) {
        contacts[index].messages = [];
      }
    }
    $("#chat-menu-link .details").hide();
    contact_list.html("");
    contact_group_list.html("");
    $("#contact-list").removeClass("in-conversation");
    $("#contact-group-list").removeClass("in-conversation");
    $(".contact-discussion ul").html("");
    $(".user-notifications").hide();
    $("#message-count").text(0);
    messagesScroller.scrollTo(0, 0);
    contactScroll.scrollTo(0, 0);
    contactGroupScroll.scrollTo(0, 0);
    contactBookScroll.scrollTo(0, 0);
    $("#invite-group-btn").hide();
    $("#leave-group-btn").hide();
  }
  if ("[]" != qs) {
    var r20 = /([\u0000-\u001f])|([\u007f-\u009f])|([\u00ad])|([\u0600-\u0604])|([\u070f])|([\u17b4-\u17b5])|([\u200c-\u200f])|([\u2028-\u202f])|([\u2060-\u206f])|([\ufeff])|([\ufff0-\uffff])+/g;
    qs = JSON.parse(qs.replace(/,\]$/, "]").replace(r20, ""));
    qs.forEach(function(item) {
      appendMessage(item.id, item.type, item.sent_date, item.received_date, item.label_value, item.label, item.labelTo, item.to_address, item.from_address, item.read, item.message, deepDataAndEvents);
    });
    if (deepDataAndEvents) {
      openConversation(contacts[current_key].address, false);
    }
    $(contacts[current_key].group ? "#contact-group-list" : "#contact-list").addClass("in-conversation");
  }
}
function appendMessage(id, string, sent, jtext, json, y, val, a, item, target, data, deepDataAndEvents) {
  var requestUrl;
  var x = "S" == string ? a : item;
  var selector = "S" == string ? item : a;
  var udataCur = "S" == string ? "(no label)" == val ? selector : val : "(no label)" == y ? x : y;
  var key = x;
  var fn = "S" == string ? selector : x;
  var recurring = false;
  if (0 === val.lastIndexOf("group_", 0) ? (requestUrl = val.replace("group_", ""), recurring = true, key = selector) : 0 === json.lastIndexOf("group_", 0) ? (requestUrl = json.replace("group_", ""), recurring = true, key = x, fn = selector) : requestUrl = udataCur, 0 === data.lastIndexOf("/invite", 0) && data.length >= 60) {
    var row = data.match(/[V79e][1-9A-HJ-NP-Za-km-z]{50,51}/g);
    var which = data.substring(61, data.length).replace(/[^A-Za-z0-9\s!?]/g, "");
    if (null != row) {
      if (string = "R") {
        return target || addInvite(row, which, id), false;
      }
      if (string = "S") {
        data = "An invite for group " + which + " has been sent.";
      }
    } else {
      if (0 == which.length) {
        which = x + "_" + String(row).substring(1, 5);
      } else {
        if (null == row) {
          data = "The group invitation was a malconfigured private key.";
        }
      }
    }
  }
  createContact(requestUrl, key, recurring);
  if (recurring) {
    createContact(udataCur, x, false, false);
    addContactToGroup(fn, key);
  }
  var o = contacts[key];
  if (0 == $.grep(o.messages, function(filter) {
    return filter.id == id;
  }).length) {
    o.messages.push({
      id : id,
      them : x,
      self : selector,
      label_msg : udataCur,
      key_msg : fn,
      group : recurring,
      message : data,
      type : string,
      sent : sent,
      received : jtext,
      read : target
    });
    o.messages.sort(function(err, connection) {
      return err.received - connection.received;
    });
    appendContact(key, false);
    if (!(current_key != key)) {
      if (!deepDataAndEvents) {
        openConversation(key, false);
      }
    }
    if ("R" == string) {
      if (0 == target) {
        addNotificationCount(key, 1);
      }
    }
  }
  if ("" == current_key) {
    current_key = key;
  }
}
function createContact(value, key, recurring, v33) {
  var data = contacts[key];
  if (void 0 == contacts[key]) {
    contacts[key] = {};
    data = contacts[key];
    data.key = key;
    data.label = value;
    data.address = key;
    data.group = recurring;
    data.addressbook = void 0 != v33 && v33;
    data.title = recurring ? "Untrusted" : data.addressbook ? "Verified" : "Unverified";
    data.avatar_type = 0;
    data.avatar = "";
    data.messages = [];
    if (recurring) {
      data.contacts = [];
    }
  }
}
function addContactToGroup(key, type) {
  return void 0 != contacts[type] && (void 0 != contacts[key] && (!existsContactInGroup(key, type) && (contacts[type].contacts.push(key), true)));
}
function existsContact(index) {
  return void 0 != contacts[index];
}
function existsContactInGroup(dataName, type) {
  return!contacts[type].contacts.indexOf(dataName) == -1;
}
function updateContactTitle(index) {
  return!!existsContact(index) && (!contacts[index].group && (!!isStaticVerified(index) && (contacts[index].title = verified_list[index].title, true)));
}
function updateContact(label, key, value, recurring) {
  recurring = recurring !== false;
  var v = contacts[key];
  if (void 0 !== v) {
    if (!(void 0 !== value && key != value)) {
      value = "";
    }
    v.messages.forEach(function(input) {
      if (!("R" !== input.type)) {
        if (!(input.them !== key && input.them !== value)) {
          input.label_msg = label;
        }
      }
    });
    if ("" === value) {
      contacts[key].label = label;
      $("#contact-book-" + key + " .contact-info .contact-name").text(label);
      $("#contact-" + key + " .contact-info .contact-name").text(label);
    } else {
      $("#contact-book-" + value + " .contact-info .contact-name").text(label);
      $("#contact-" + value + " .contact-info .contact-name").text(label);
    }
    if (openConversation) {
      openConversation(key, true);
    }
  }
}
function appendContact(key, v33, recurring) {
  var entry = recurring ? "contact-book-" : "contact-";
  var $option = $("#" + entry + key);
  var opt = contacts[key];
  if (0 === $option.length) {
    var message = "";
    if (!(void 0 === opt.messages[0])) {
      if (!recurring) {
        message = opt.messages[0].message;
      }
    }
    var lineSeparator = "<li id='" + entry + key + "' class='contact' data-title='" + opt.label + "'>                <span class='contact-info'>                    <span class='contact-name'>" + (opt.group && recurring ? "<i class='fa fa-users' style='padding-right: 7px;'></i>" : "") + opt.label + "</span>                    <span class='" + (recurring ? "contact-address" : "contact-message") + "'>" + (recurring ? opt.address : message) + "</span>                </span>                <span class='contact-options'>                        <span class='message-notifications'>0</span> <span class='delete' onclick='deleteMessages(\"" +
    key + "\")'><i class='fa fa-minus-circle'></i></span>                        </span></li>";
    if (recurring) {
      contact_book_list.append(lineSeparator);
      $("#" + entry + key).find(".delete").hide();
    } else {
      if (opt.group) {
        contact_group_list.append(lineSeparator);
      } else {
        contact_list.append(lineSeparator);
      }
    }
    $option = $("#" + entry + key).selection("li").on("dblclick", function(dataAndEvents) {
      openConversation(key, true);
      prependContact(key);
    }).on("click", function(dataAndEvents) {
      openConversation(key, true);
    });
    if (recurring) {
      $option.on("click", function(dataAndEvents) {
      });
    }
    $option.find(".delete").on("click", function(event) {
      event.stopPropagation();
    });
    $option.find(".message-notifications").hide();
  } else {
    if (!(void 0 === opt.messages)) {
      if (!recurring) {
        $("#" + entry + key + " .contact-info .contact-message").text(opt.messages[opt.messages.length - 1].message);
      }
    }
  }
  if (v33) {
    openConversation(key, false);
  }
}
function getContactUsername(i) {
  var line;
//TODO: SIGNAL bridge
  return "object" == typeof verified_list[i] ? verified_list[i].username : (line = bridge.getAddressLabel(i), "string" == typeof line ? line.replace("group_", "") : i);
}
function isStaticVerified(property) {
  return "object" == typeof verified_list[property];
}
function allowCustomAvatar(i) {
  return "object" == typeof verified_list[i] && ("boolean" == typeof verified_list[i].custom_avatar && verified_list[i].custom_avatar);
}
function getIconTitle(value) {
  return "unverified" == value ? "fa fa-cross " : "verified" == value ? "fa fa-check " : "contributor" == value ? "fa fa-cog " : "spectreteam" == value ? "fa fa-code " : "";
}
function addNotificationCount(id, i) {
  if (void 0 == contacts[id]) {
    return false;
  }
  var script = $("#contact-" + id).find(".message-notifications");
  var cDigit = script.html();
  script.text(parseInt(cDigit) + parseInt(i)).show();
  $("#chat-menu-link .details").show();
  $(".user-notifications").show();
  $("#message-count").text(parseInt($("#message-count").text()) + 1).show();
  $("#contact-" + id).prependTo(contacts[id].group ? "#contact-group-list ul" : "#contact-list ul");
}
function removeNotificationCount(key) {
  if (void 0 == key) {
    if ("" !== current_key) {
      key = current_key;
    }
  }
  messagesScroller.refresh();
  var results = contacts[key];
  if (void 0 == results) {
    return false;
  }
  var status = $("#contact-" + key).find(".message-notifications");
  var xshift = status.html();
  if (0 == status.text()) {
    return false;
  }
  status.text(0);
  status.hide();
  var $label = $("#message-count");
  var originalLabel = parseInt($label.text()) - xshift;
  if ($label.text(originalLabel), 0 == originalLabel ? ($label.hide(), $("#chat-menu-link .details").hide()) : $label.show(), 0 == results.messages.length) {
    return 0;
  }
  var i = results.messages.length;
  for (;i--;) {
    if (!results.messages[i].read) {
      bridge.markMessageAsRead(results.messages[i].id);
    }
  }
}
function openConversation(key, recurring) {
  function convert(src) {
    return micromarkdown.parse(emojione.toImage(src)).replace(/<a class="mmd_spectrecash" href="(.+)">(.+)<\/a>/g, '<a class="mmd_spectrecash" onclick="return confirmConversationOpenLink()" target="_blank" href="$1" data-title="$1">$1</a>');
  }
  if (recurring) {
    $("#chat-menu-link").click();
  }
  current_key = key;
  var tagList = $(".contact-discussion ul");
  var self = contacts[key];
  tagList.html("");
  var len = self.group;
  if (len) {
    $("#invite-group-btn").show();
  } else {
    $("#invite-group-btn").hide();
    $("#leave-group-btn").hide();
  }
  $("#chat-header").text(self.label).addClass("editable");
  $("#chat-header").data("value", self.label);
  $("#chat-header").off();
  $("#chat-header").on("dblclick", function(event) {
    event.stopPropagation();
    updateValueChat($(this), self.key);
  }).attr("data-title", "Double click to edit").tooltip();
  var params;
  var i = false;
  if (recurring) {
    removeNotificationCount(self.key);
  }
  self.messages.forEach(function(opts, dataAndEvents) {
    if (dataAndEvents > 0 && combineMessages(params, opts)) {
      return $("#" + params.id).attr("id", opts.id), $("#" + opts.id + " .message-text").append(convert(opts.message)), void(params = opts);
    }
    params = opts;
    var d = new Date(1E3 * opts.sent);
    var date = new Date(1E3 * opts.received);
    addAvatar(opts.them);
    var failureMessage = opts.label_msg == opts.key_msg ? " data-toggle=\"modal\" data-target=\"#add-address-modal\" onclick=\"clearSendAddress(); $('#add-rcv-address').hide(); $('#add-send-address').show(); $('#new-send-address').val('" + opts.key_msg + "')\" " : "";
    tagList.append("<li id='" + opts.id + "' class='message-wrapper " + ("S" == opts.type ? "my-message" : "other-message") + "' contact-key='" + self.key + "'>                <span class='message-content'>                    <span class='info'>" + getAvatar("S" == opts.type ? opts.self : opts.them) + "</span>                    <span class='user-name' " + failureMessage + ">" + opts.label_msg + "                    </span>                    <span class='title'>                    </span>                    <span class='timestamp'>" +
    ((d.getHours() < 10 ? "0" : "") + d.getHours() + ":" + (d.getMinutes() < 10 ? "0" : "") + d.getMinutes() + ":" + (d.getSeconds() < 10 ? "0" : "") + d.getSeconds()) + "</span>                       <span class='delete' onclick='deleteMessages(\"" + self.key + '", "' + opts.id + "\");'><i class='fa fa-minus-circle'></i></span>                       <span class='message-text'>" + convert(opts.message) + "</span>                </span>             </li>");
    $("#" + opts.id + " .timestamp").attr("data-title", "Sent: " + d.toLocaleString() + "\n Received: " + date.toLocaleString()).tooltip().find(".message-text").tooltip();
    insertTitleHTML(opts.id, opts.key_msg);
    if ("S" == opts.type) {
      $("#" + opts.id + " .user-name").attr("data-title", "" + opts.self).tooltip();
      if (opts.group) {
        if (!i) {
          $("#message-from-address").val(opts.self);
          $("#message-to-address").val(opts.them);
        }
      }
    } else {
      $("#" + opts.id + " .user-name").attr("data-title", "" + opts.them).tooltip();
    }
    if (!i && self.messages.length > 0) {
      if (len) {
        if ("R" == opts.type) {
          $("#message-to-address").val(opts.self);
        }
      } else {
        $("#message-from-address").val(opts.self);
        $("#message-to-address").val(opts.them);
      }
    } else {
      if (0 == self.messages.length) {
        $(".contact-discussion ul").html("<li id='remove-on-send'>Starting Conversation with " + self.label + " - " + self.address + "</li>");
        $("#message-to-address").val(self.address);
      }
    }
  });
  setTimeout(function() {
    scrollMessages();
  }, 200);
}
function insertTitleHTML(lhs, index) {
  if (!existsContact(index)) {
    return false;
  }
  var udataCur = (contacts[index], contacts[index].title.toLowerCase());
  $("#" + lhs + " .title").addClass(getIconTitle(udataCur) + udataCur + "-mark");
  $("#" + lhs + " .title").hover(function() {
    $(this).text(" " + udataCur);
  }, function() {
    $(this).text("");
  });
}
function confirmConversationOpenLink() {
  return confirm("Are you sure you want to open this link?\n\nIt will leak your IP address and other browser metadata, the least we can do is advice you to copy the link and open it in a _Tor Browser_ instead.\n\n You can disable this message in options.");
}
function combineMessages(info, self) {
  return info.type == self.type && ("R" == self.type && info.them == self.them || "S" == self.type && info.self == self.self);
}
function addRandomAvatar(i) {
  return!!existsContact(i) && (contacts[i].avatar_type = 1, void(contacts[i].avatar = generateRandomAvatar(i)));
}
function generateRandomAvatar(i) {
  var sha = new jsSHA("SHA-512", "TEXT");
  sha.update(i);
  var block = sha.getHash("HEX");
  var n = (new Identicon(block, 40)).toString();
  return'<img width=40 height=40 src="data:image/png;base64,' + n + '">';
}
function addCustomAvatar(i) {
  contacts[i].avatar_type = 2;
  contacts[i].avatar = '<img width=40 height=40 src="qrc:///assets/img/avatars/' + contacts[i].label + '.png">';
}
function addAvatar(classNames) {
  if (allowCustomAvatar(classNames)) {
    addCustomAvatar(classNames);
  } else {
    addRandomAvatar(classNames);
  }
}
function getAvatar(i) {
  return allowCustomAvatar(i) ? '<img width=40 height=40 src="qrc:///assets/img/avatars/' + verified_list[i].username + '.png">' : existsContact(i) ? contacts[i].avatar : generateRandomAvatar(i);
}
function prependContact(key) {
  var c = contacts[key];
  if (c.group) {
    $("#contact-group-list").addClass("in-conversation");
    $("#contact-" + key).prependTo($("#contact-group-list ul"));
  } else {
    $("#contact-list").addClass("in-conversation");
    $("#contact-" + key).prependTo($("#contact-list ul"));
  }
}
function createGroupChat() {
  var key = $("#new-group-name").val();
  if ("" == key) {
    return false;
  }
  $("#filter-new-group").text("");
  $("#new-group-modal").modal("hide");
  bridge.createGroupChat(key);
}
function createGroupChatResult(result) {
    var camelKey = result;
    inviteGroupChat(camelKey);
    createContact(key, camelKey, true);
    appendContact(camelKey, true, false);
}

function addInvite(row, callback, userId) {
  return 0 == $("#invite-" + row + "-" + userId).length && void $("#group-invite-list").append("<div id='invite-" + row + "-" + userId + '\'><a class=\'group-invite\'><i class="fa fa-envelope"></i><span class="group-invite-label"> ' + callback + ' </span><i class="fa fa-check group-invite-check" onclick="acceptInvite(\'' + row + "','" + callback + "', '" + userId + '\');"></i><i class="fa fa-close group-invite-close" onclick="deleteInvite(\'' + row + "','" + userId + "');\"></i></a></div>");
}
function deleteInvite(keepData, message) {
  bridge.deleteMessage(message);
  $("#invite-" + keepData + "-" + message).html("");
}
function acceptInvite(key, data, endpoint) {
  endpointAcceptInvite = endpoint;
  dataAcceptInvite = data;
  bridge.joinGroupChat(key, data)
  return "false";
}
function resetAcceptInviteVariables() {
  endpointAcceptInvite = null;
  dataAcceptInvite = null;
}
function joinGroupChatResult(result) {
  var endpoint = endpointAcceptInvite;
  var data = dataAcceptInvite;
  deleteInvite(key, endpoint);
  var camelKey = result;
  resetAcceptInviteVariables();
  return "false" !== camelKey && (updateContact(data, camelKey), createContact(data, camelKey, true), void appendContact(camelKey, true, false));
}
function inviteGroupChat(data) {
  var out = [];
  var target = "#invite-modal-tbody";
  if (void 0 != data) {
    target = "group-modal-tbody";
  } else {
    data = current_key;
  }
  $(target + " tr").each(function() {
    var copies = $(this).find(".address").text();
    var a = $(this).find(".invite .checkbox").is(":checked");
    if (a) {
      out.push(copies);
      $(this).find(".invite .checkbox").attr("checked", false);
    }
  });
  var result = [];
  if (out.length > 0) {
    result = bridge.inviteGroupChat(data, out, $("#message-from-address").val());//TODO: SIGNAL bridge
  }
  result.length > 0;
}
function leaveGroupChat() {
//TODO: SIGNAL bridge ---- not being called anywhere
    var leaveGroupChat = bridge.leaveGroupChat(current_key);
  return leaveGroupChat;
}
function openInviteModal() {
  if (0 == current_key.length) {
    return false;
  }
  bridge.getAddressLabelAsync(current_key);
}
function getAddressLabelResultGroup(result) {
    var display_id = result.replace("group_", "");
    $("#existing-group-name").val(display_id);
}
function submitInviteModal() {
  inviteGroupChat();
  $("#invite-to-group-modal").hide();
}
function scrollMessages() {
  messagesScroller.refresh();
  var loop = function() {
    var posY = messagesScroller.y;
    messagesScroller.refresh();
    if (posY !== messagesScroller.maxScrollY) {
      messagesScroller.scrollTo(0, messagesScroller.maxScrollY, 100);
    }
  };
  setTimeout(loop, 100);
}
function newConversation() {
  var camelKey = $("#new-contact-address").val();
  var udataCur = $("#new-contact-name").val();
    //TODO: SIGNAL bridge x 4
  return createContact(udataCur, camelKey, false), result = bridge.newAddress($("#new-contact-name").val(), 0, $("#new-contact-address").val(), true), "" === result && "Duplicate Address." !== bridge.lastAddressError() ? void $("#new-contact-address").css("background", "#E51C39").css("color", "white") : ($("#new-contact-address").css("background", "").css("color", ""), bridge.setPubKey($("#new-contact-address").val(), $("#new-contact-pubkey").val()), bridge.updateAddressLabel($("#new-contact-address").val(),
  $("#new-contact-name").val()), $("#new-contact-modal").modal("hide"), $("#message-to-address").val($("#new-contact-address").val()), $("#message-text").focus(), $("#new-contact-address").val(""), $("#new-contact-name").val(""), $("#new-contact-pubkey").val(""), $("#contact-list ul li").removeClass("selected"), $("#contact-list").addClass("in-conversation"), $("#contact-group-list ul li").removeClass("selected"), $("#contact-group-list").addClass("in-conversation"), void setTimeout(function() {
    openConversation(camelKey, true);
    $(".contact-discussion ul").html("<li id='remove-on-send'>Starting Conversation with " + camelKey + " - " + udataCur + "</li>");
  }, 1E3));
}
function sendMessage() {
  $("#remove-on-send").remove();
  bridge.sendMessage(current_key, $("#message-text").val(), $("#message-from-address").val());
}
function sendMessageResult(result) {
    if (result) {
      $("#message-text").val("");
    }
}

function deleteMessages(key, id) {
  var meta = contacts[key];
  if (!confirm("Are you sure you want to delete " + (void 0 == id ? "these messages?" : "this message?"))) {
    return false;
  }
  var script = $("#message-count");
  parseInt(script.text());
  removeNotificationCount(key);
  if (void 0 == id) {
    current_key = "";
  }
  var i = 0;
  for (;i < meta.messages.length;i++) {
    if (void 0 === id) {
      if (!bridge.deleteMessage(meta.messages[i].id)) {//TODO: SIGNAL bridge
        return false;
      }
      $("#" + meta.messages[i].id).remove();
      meta.messages.splice(i, 1);
      i--;
    } else {
      if (meta.messages[i].id === id) {
        if (bridge.deleteMessage(id)) { //TODO: SIGNAL bridge
          $("#" + id).remove();
          meta.messages.splice(i, 1);
          i--;
          break;
        }
        return false;
      }
    }
  }
  if (0 == meta.messages.length) {
    $("#contact-" + key).remove();
    $("#contact-list").removeClass("in-conversation");
    $("#contact-group-list").removeClass("in-conversation");
  } else {
    iscrollReload();
    openConversation(key, false);
  }
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
  contactScroll.refresh();
  contactGroupScroll.refresh();
  contactBookScroll.refresh();
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
  setType : function(v) {
    switch(this.type = void 0 == v ? 0 : v, v) {
      case 1:
        this.name = "mXSPEC";
        this.display = "mXSPEC";
        break;
      case 2:
        this.name = "uXSPEC";
        this.display = "&micro;XSPEC";
        break;
      case 3:
        this.name = "sXSPEC";
        this.display = "spectoshi";
        break;
      default:
        this.name = this.display = "XSPEC";
    }
    $("td.unit,span.unit,div.unit").html(this.display);
    $("select.unit").val(v).trigger("change");
    $("input.unit").val(this.name);
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
      $("#total-big .cents").text(data[1]);
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
    target = this[target];
    if (0 == name) {
      target.html("");
      target.parent("tr").hide();
    } else {
      target.text(unit.format(name));
      target.parent("tr").show();
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
      return "<tr><td class='text-left' style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><center><label style='margin-top:6px;' class='label label-important inline fs-12'>" + ("input" == data.t ? "Received" : "output" == data.t ? "Sent" : "inout" == data.t ? "In-Out" : "Stake") + "</label></center></td><td class='text-left' style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><center><a id='" + data.id.substring(0, 17) + "' data-title='" + data.tt +
      "' href='#' onclick='$(\"#navitems [href=#transactions]\").click();$(\"#" + data.id + "\").click();'> " + unit.format(data.am) + " " + unit.display + " </a></center></td><td style='border-top: 1px solid rgba(230, 230, 230, 0.7);border-bottom: none;'><span class='overview_date' data-value='" + data.d + "'><center>" + data.d_s + "</center></span></td></tr>";
    };
    var idfirst = message.id.substring(0, 17);
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
var Name = "You";
var initialAddress = true;
var Transactions = [];
var filteredTransactions = [];
var contacts = {};
var contact_list;
var contact_group_list;
var contact_book_list;
var current_key = "";
var verified_list = {
  "specdev-slack" : {
    username : "specdev-slack",
    title : "Spectreteam",
    custom_avatar : false
  },
  SR46wGPK5sGwT9qymRNTVtF9ExHHvVuDXQ : {
    username : "crz",
    title : "Spectreteam",
    custom_avatar : false
  },
  SVY9s4CySAXjECDUwvMHNM6boAZeYuxgJE : {
    username : "kewde",
    title : "Spectreteam",
    custom_avatar : true
  },
  dasource : {
    username : "dasource",
    title : "Spectreteam",
    custom_avatar : false
  },
  SNLYNVwWQNgPqxND5iWyRfnGbEPnvSGVLw : {
    username : "ffmad",
    title : "Spectreteam",
    custom_avatar : false
  },
  STWYshQBdzk47swrp2S77jHLxjrNAWUNdq : {
    username : "ludx",
    title : "Spectreteam",
    custom_avatar : false
  },
  "edu-online" : {
    username : "edu-online",
    title : "Spectreteam",
    custom_avatar : false
  },
  arcanum : {
    username : "arcanum",
    title : "Spectreteam",
    custom_avatar : false
  },
  SQqVGXi9Hi1CJv7Qy4gjvxyVinemTx8nK7 : {
    username : "allien",
    title : "Spectreteam",
    custom_avatar : false
  },
  sebsebastian : {
    username : "sebsebastian",
    title : "Spectreteam",
    custom_avatar : false
  },
  SPXkEj2Daa9un5uzKHFNpseAfirsygCAhq : {
    username : "litebit",
    title : "Contributor",
    custom_avatar : true
  },
  SZxH6HNYAh9iNaGLdoHYjSN2qWvfjahrF1 : {
    username : "6ea86b96",
    title : "Verified"
  },
  ShKkz1b6XD4ASgTP9BAh8C3zi4Z9HsCH5F : {
    username : "dadon",
    title : "Verified"
  },
  ScrvNCexThmfctYcLZLwzFCcaH6znW69sj : {
    username : "dzarmush",
    title : "Verified"
  },
  SZ8bMXxkBELD6s5jSsBRLCwvkXibwRWw4q : {
    username : "GRE3N",
    title : "Verified"
  },
  SPAfq2i8cP1SMcaTT8nMTxa2Fg9LNNJSyk : {
    username : "NGS",
    title : "Verified"
  },
  SWUBRJUdgck6d8tiM5hf4wEAAp3J8JyuQj : {
    username : "The-C-Word",
    title : "Verified"
  },
  SgyxAj1j2ebtecYAFu5McPyzZUqDX3UpBP : {
    username : "tintifax",
    title : "Verified"
  },
  SWG4eCfpsrFwB64owwrLjvDnyYkdCp2oPi : {
    username : "wwonka36",
    title : "Verified"
  }
};

var contactScroll;
var contactGroupScroll;
var contactBookScroll;
var messagesScroller;

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
  spectreChatInit();
  chainDataPage.init();
  walletManagementPage.init();

  contactScroll = new IScroll("#contact-list", {
    mouseWheel : true,
    lockDirection : true,
    scrollbars : true,
    interactiveScrollbars : true,
    scrollbars : "custom",
    scrollY : true,
    scrollX : false,
    preventDefaultException : {
      tagName : /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/
    }
  });
  contactGroupScroll = new IScroll("#contact-group-list", {
    mouseWheel : true,
    lockDirection : true,
    scrollbars : true,
    interactiveScrollbars : true,
    scrollbars : "custom",
    scrollY : true,
    scrollX : false,
    preventDefaultException : {
      tagName : /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/
    }
  });
  contactBookScroll = new IScroll("#contact-book-list", {
    mouseWheel : true,
    lockDirection : true,
    scrollbars : true,
    interactiveScrollbars : true,
    scrollbars : "custom",
    scrollY : true,
    scrollX : false,
    preventDefaultException : {
      tagName : /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/
    }
  });
  messagesScroller = new IScroll(".contact-discussion", {
    mouseWheel : true,
    lockDirection : true,
    scrollbars : true,
    interactiveScrollbars : true,
    scrollbars : "custom",
    scrollY : true,
    scrollX : false,
    preventDefaultException : {
      tagName : /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/
    }
  });

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
