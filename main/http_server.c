/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <sys/param.h>
//#include "nvs_flash.h"
#include "esp_netif.h"
//#include "esp_eth.h"
//#include "protocol_examples_common.h"
#include "esp_ota_ops.h"
#include <esp_sleep.h>

#include <esp_http_server.h>

//#include "pages.h"
#include "router_globals.h"
#include "cmd_system.h"
#include "nvs.h"
#include "lwip/lwip_napt.h"

extern char* appName;
extern char* appVersion;
extern char* firmwareUpdatePassword;
extern bool has_static_ip;
extern char* ap_ip;
struct portmap_table_entry {
  u32_t daddr;
  u16_t mport;
  u16_t dport;
  u8_t proto;
  u8_t valid;
};
extern struct portmap_table_entry portmap_tab[IP_PORTMAP_MAX];

static const char *TAG = "HTTPServer";

esp_timer_handle_t restart_timer;

#define uS_TO_S_FACTOR 1000000LL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP2S  2        // Time ESP32 will sleep (in seconds)

static void restart_timer_callback(void* arg)
{
    ESP_LOGI(TAG, "Restarting now...");
    esp_restart();
}

esp_timer_create_args_t restart_timer_args = {
        .callback = &restart_timer_callback,
        /* argument specified here will be passed to timer callback function */
        .arg = (void*) 0,
        .name = "restart_timer"
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Page not found");
    return ESP_FAIL;
}


char * the_page = NULL;  // used to hold complete response page before its sent to browser

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

static esp_err_t index_get_handler(httpd_req_t *req) {

  //printf("In index_get_handler\n");

  char param_ssid[64] = {};    
  char param_ssidpw[64] = {};
  char param_staticip[20] = {};
  char param_subnetmask[20] = {};
  char param_gateway[20] = {};
  char param_otapw[64] = {};
  char param_delete[10] = {}; // has mport
  char param_proto[12] = {};
  char param_add[10] = {};  // has mport
  char param_nproto[12] = {};
  char param_dip[20] = {};
  char param_dport[10] = {};
  char param_reset[64] = {};
  char param_reboot[64] = {};

  size_t buf_len;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  char * buf = (char *) malloc(buf_len);
  if (buf_len > 1 && httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {

    // process setting network if info is there
    if (httpd_query_key_value(buf, "ssid", param_ssid, sizeof(param_ssid)) != ESP_OK) 
      strcpy(param_ssid,"");
    if (httpd_query_key_value(buf, "ssidpw", param_ssidpw, sizeof(param_ssidpw)) != ESP_OK) 
      strcpy(param_ssidpw,"");
    if (httpd_query_key_value(buf, "staticip", param_staticip, sizeof(param_staticip)) != ESP_OK) 
      strcpy(param_staticip,"");
    if (httpd_query_key_value(buf, "subnetmask", param_subnetmask, sizeof(param_subnetmask)) != ESP_OK) 
      strcpy(param_subnetmask,"");
    if (httpd_query_key_value(buf, "gateway", param_gateway, sizeof(param_gateway)) != ESP_OK) 
      strcpy(param_gateway,"");
    if (httpd_query_key_value(buf, "otapw", param_otapw, sizeof(param_otapw)) != ESP_OK) 
      strcpy(param_otapw,"");
    if (httpd_query_key_value(buf, "delete", param_delete, sizeof(param_delete)) != ESP_OK) 
      strcpy(param_delete,"");
    if (httpd_query_key_value(buf, "proto", param_proto, sizeof(param_proto)) != ESP_OK) 
      strcpy(param_proto,"");
    if (httpd_query_key_value(buf, "add", param_add, sizeof(param_add)) != ESP_OK) 
      strcpy(param_add,"");
    if (httpd_query_key_value(buf, "nproto", param_nproto, sizeof(param_nproto)) != ESP_OK) 
      strcpy(param_nproto,"");
    if (httpd_query_key_value(buf, "dip", param_dip, sizeof(param_dip)) != ESP_OK) 
      strcpy(param_dip,"");
    if (httpd_query_key_value(buf, "dport", param_dport, sizeof(param_dport)) != ESP_OK) 
      strcpy(param_dport,"");
    if (httpd_query_key_value(buf, "reset", param_reset, sizeof(param_reset)) != ESP_OK) 
      strcpy(param_reset,"");
    if (httpd_query_key_value(buf, "reboot", param_reboot, sizeof(param_reboot)) != ESP_OK) 
      strcpy(param_reboot,"");
/*
    printf("param_ssid is %s, %d\n", param_ssid, strlen(param_ssid));
    printf("param_ssidpw is %s, %d\n", param_ssidpw, strlen(param_ssidpw));
    printf("param_staticip is %s, %d\n", param_staticip, strlen(param_staticip));
    printf("param_subnetmask is %s, %d\n", param_subnetmask, strlen(param_subnetmask));
    printf("param_gateway is %s, %d\n", param_gateway, strlen(param_gateway));
    printf("param_otapw is %s, %d\n", param_otapw, strlen(param_otapw));
    printf("param_delete is %s, %d\n", param_delete, strlen(param_delete));
    printf("param_proto is %s, %d\n", param_proto, strlen(param_proto));
    printf("param_add is %s, %d\n", param_add, strlen(param_add));
    printf("param_nproto is %s, %d\n", param_nproto, strlen(param_nproto));
    printf("param_dip is %s, %d\n", param_dip, strlen(param_dip));
    printf("param_dport is %s, %d\n", param_dport, strlen(param_dport));
    printf("param_reset is %s, %d\n", param_reset, strlen(param_reset));
    printf("param_reboot is %s, %d\n", param_reboot, strlen(param_reboot));
*/
    // process network config
    if (strlen(param_ssid) > 0 && strlen(param_ssidpw) > 0 && strlen(param_staticip) > 0 && 
      strlen(param_subnetmask) > 0 && strlen(param_gateway) > 0 && strlen(param_otapw) > 0) { 

      nvs_handle_t nvs;
      nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
      nvs_set_str(nvs, "ota_pw", param_otapw);
      nvs_close(nvs);
      strcpy(ota_pw, param_otapw); // saved new ota password

      strcpy(buf, param_ssid);  // reusing buf to add <NEW> to ssid
      strcpy(param_ssid,"<NEW>"); 
      strcat(param_ssid, buf);  // using <NEW> as a flag to indicate trying new network info
      int argc = 3;
      char *argv[4];
      argv[0] = "set_sta";
      argv[1] = param_ssid;
      argv[2] = param_ssidpw;
      set_sta(argc, argv);

      argc = 4;
      argv[0] = "set_sta_static";
      argv[1] = param_staticip;
      argv[2] = param_subnetmask;
      argv[3] = param_gateway;
      set_sta_static(argc, argv);

      // set new STA static ip - rebooting to take effect
      delay(1000);
      esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP2S * uS_TO_S_FACTOR);
      esp_deep_sleep_start();
    }

    // process delete port forwarding rule if info is there
    if (strlen(param_delete) > 0 && strlen(param_proto) > 0 && strlen(param_otapw) > 0) { // delete a port forward rule 
      if (strcmp(param_otapw, ota_pw) != 0) {
        strcpy(the_page, "Password is invalid");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      } else {
        uint8_t tcp_udp;
        if (strcmp(param_proto,"PROTO_TCP") == 0) {
          tcp_udp = PROTO_TCP;
        } else {
          tcp_udp = PROTO_UDP;
        }
        del_portmap(tcp_udp, (u16_t) atoi(param_delete));
        strcpy(the_page, "Portmapped rule deleted");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      }
    }

    // process add port forwarding rule if info is there
    if (strlen(param_add) > 0 && strlen(param_nproto) > 0 && strlen(param_dip) > 0 && strlen(param_dport) > 0 && strlen(param_otapw) > 0) { // add a port forward rule 
      if (strcmp(param_otapw, ota_pw) != 0) {
        strcpy(the_page, "Password is invalid");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      } else {
        uint8_t tcp_udp;
        if (strcmp(param_nproto,"PROTO_TCP") == 0) {
          tcp_udp = PROTO_TCP;
        } else {
          tcp_udp = PROTO_UDP;
        }
        add_portmap(tcp_udp, (u16_t) atoi(param_add), ipaddr_addr(param_dip), (u16_t) atoi(param_dport));
        strcpy(the_page, "Portmapped rule add");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      }
    }

    // process factory reset
    if (strlen(param_reset) > 0) { //  
      if (strcmp(param_reset, ota_pw) != 0) {
        strcpy(the_page, "Password is invalid");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      } else {
        // clear nvs esp32_lpf and reboot
        nvs_handle_t nvs;
        esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
        if (err == ESP_OK) {
          err = nvs_erase_all(nvs);
          if (err == ESP_OK) {
            err = nvs_commit(nvs);
            sprintf(the_page, "Factory reset in progress...\nWait a few seconds then connect at %s", ap_ip);
            httpd_resp_send(req, the_page, strlen(the_page));
            delay(1000);
            esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP2S * uS_TO_S_FACTOR);
            esp_deep_sleep_start();
          }  
        }
      }
      sprintf(the_page, "Failed to erase config info, all should be as before");
      httpd_resp_send(req, the_page, strlen(the_page));
      return ESP_OK;
    }

    // process reboot
    if (strlen(param_reboot) > 0) { //  
      if (strcmp(param_reboot, ota_pw) != 0) {
        strcpy(the_page, "Password is invalid");
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      } else {
        sprintf(the_page, "Rebooting...  Wait a few seconds then connect at %s", static_ip);
        httpd_resp_send(req, the_page, strlen(the_page));
        delay(1000);
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP2S * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
      }
    }

    strcpy(the_page, "Bad command");
    httpd_resp_send(req, the_page, strlen(the_page));
    return ESP_OK;
  }

  const char msg1[] = "<!doctype html> \n\
<html> \n\
<head> \n\
<meta charset=\"utf-8\"> \n\
<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> \n\
<title>ESP32LocalPortForwarderConfig</title> \n\
<script> \n\
function do_netconfig() { \n\
  var a = document.getElementById(\"ssid\").value; \n\
  var b = document.getElementById(\"ssidpw\").value; \n\
  var c = document.getElementById(\"staticip\").value; \n\
  var d = document.getElementById(\"subnetmask\").value; \n\
  var e = document.getElementById(\"gateway\").value; \n\
  var f = document.getElementById(\"otapw\").value; \n\
  if ((a == null || a.trim() == \"\") || (b == null || b.trim() == \"\") || (c == null || c.trim() == \"\") || (d == null || d.trim() == \"\") || (e == null || e.trim() == \"\") || (f == null || f.trim() == \"\")) { \n\
    alert( \"All fields must have valid values\" ); \n\
  } else { \n\
    document.getElementById('config').innerHTML = '<h1>ESP32 Local Port Forwarder</h1>The new settings have been sent to the device.<br/>This page will refresh in 20 seconds...'; \n\
    var XHR = new XMLHttpRequest(); \n\
    XHR.open( \"GET\", `/?ssid=${a.trim()}&ssidpw=${b.trim()}&staticip=${c.trim()}&subnetmask=${d.trim()}&gateway=${e.trim()}&otapw=${f.trim()}`, true ); \n\
    XHR.send( null ); \n\
    setTimeout(function() { \n\
      location.href = \"http://\"+c.trim()+\"/\"; \n\
      }, 20000); \n\
  } \n\
} \n\
function do_delete(proto,mport) { \n\
  var a = document.getElementById(\"otapw\").value; \n\
  if ((a == null || a.trim() == \"\")) { \n\
    alert( \"OTA password must be entered\" ); \n\
  } else { \n\
    var XHR = new XMLHttpRequest(); \n\
    XHR.open( \"GET\", `/?delete=${mport}&proto=${proto}&otapw=${a.trim()}`, true ); \n\
      XHR.onloadend = function(){ \n\
        alert( XHR.responseText ); \n\
        location.href = \"/\"; \n\
      } \n\
    XHR.send( null ); \n\
  } \n\
} \n\
function do_add() { \n\
  var a = document.getElementById(\"otapw\").value; \n\
  var b = document.getElementById(\"nproto\").value; \n\
  var c = document.getElementById(\"port\").value; \n\
  var d = document.getElementById(\"dip\").value; \n\
  var e = document.getElementById(\"dport\").value; \n\
  if ((a == null || a.trim() == \"\")) { \n\
    alert( \"OTA password must be entered\" ); \n\
  } else { \n\
    if ((b == null || b.trim() == \"\") || (c == null || c.trim() == \"\") || (d == null || d.trim() == \"\") || (e == null || e.trim() == \"\")) { \n\
      alert( \"All fields must have valid values\" ); \n\
    } else { \n\
      var XHR = new XMLHttpRequest(); \n\
      XHR.open( \"GET\", `/?add=${c.trim()}&nproto=${b}&dip=${d.trim()}&dport=${e.trim()}&otapw=${a.trim()}`, true ); \n\
        XHR.onloadend = function(){ \n\
          alert( XHR.responseText ); \n\
          location.href = \"/\"; \n\
        } \n\
      XHR.send( null ); \n\
    } \n\
  } \n\
} \n\
function do_updatefirmware() { \n\
  location.href = \"/updatefirmware\"; \n\
} \n\
function do_reboot() { \n\
  var a = document.getElementById(\"otapw\").value; \n\
  if ((a == null || a.trim() == \"\")) { \n\
    alert( \"OTA password must be entered\" ); \n\
  } else { \n\
    let confirmAction = confirm(\"The ESP32 Local Port Forwarder will be rebooted.  Are you sure?\"); \n\
    if (confirmAction) { \n\
      var XHR = new XMLHttpRequest(); \n\
      XHR.open( \"GET\", `/?reboot=${a.trim()}`, true ); \n\
        XHR.onloadend = function(){ \n\
          alert( XHR.responseText ); \n\
        } \n\
      XHR.send( null ); \n\
      setTimeout(function() { \n\
        location.href = \"/\"; \n\
      }, 8000); \n\
    } else { \n\
      alert(\"Reboot Cancelled\"); \n\
    } \n\
  } \n\
} \n\
function do_factoryreset() { \n\
  var a = document.getElementById(\"otapw\").value; \n\
  if ((a == null || a.trim() == \"\")) { \n\
    alert( \"OTA password must be entered\" ); \n\
  } else { \n\
    let confirmAction = confirm(\"All settings will be deleted and the network will \\nneed to be reconfigured at %s.  Are you sure?\"); \n\
    if (confirmAction) { \n\
      var XHR = new XMLHttpRequest(); \n\
      XHR.open( \"GET\", `/?reset=${a.trim()}`, true ); \n\
        XHR.onloadend = function(){ \n\
          alert( XHR.responseText ); \n\
        } \n\
      XHR.send( null ); \n\
    } else { \n\
      alert(\"Factory Reset Cancelled\"); \n\
    } \n\
  } \n\
//  location.href = \"/updatefirmware\"; \n\
} \n\
</script> \n\
</head> \n\
<body> \n\
<center> \n\
<div id='config'> \n\
<h1>ESP32 Local Port Forwarder</h1> \n\
 \n";
//  strcpy(the_page, msg1);
  sprintf(the_page, msg1, ap_ip);
  // send first chunk
  httpd_resp_send_chunk(req, the_page, strlen(the_page));

  if (has_static_ip) {  // show port forward config
    strcpy(the_page, "Enter the OTA Firmware Update Password to make changes or reset <input type=\"password\" id=\"otapw\" maxlength=\"20\"><br/><br/>\n");

    int rulecount = 0;
    char buf[100];

    for (int i = 0; i<IP_PORTMAP_MAX; i++) {
        if (portmap_tab[i].valid) { rulecount++; }
    }

    if (rulecount >= IP_PORTMAP_MAX) { // max rules set
        sprintf (buf,"Max of %d rules set, delete an existing rule to add a new rule<br/><br/>\n", IP_PORTMAP_MAX);
        strcat(the_page,buf);
    } else {
        strcat(the_page, "Add a new Portmap Rule<br/><table border=\"1px\" cellpadding=\"3px\">\n"); 
        strcat(the_page, "<tr><td>&nbsp;Proto</td><td>&nbsp;Port</td><td>&nbsp;Dest IP</td><td>&nbsp;Dest Port</td><tr>\n"); 
        strcat(the_page, "<tr><td><select id=\"nproto\"><option value=\"PROTO_TCP\">TCP</option><option value=\"PROTO_UDP\">UDP</option></select></td><td><input type=\"text\" id=\"port\" maxlength=\"5\"></td><td><input type=\"text\" id=\"dip\" maxlength=\"15\"></td><td><input type=\"text\" id=\"dport\" maxlength=\"5\"></td><td><input type=\"button\" value=\"Add\" onclick=\"do_add();\"></td><tr>\n"); 
        strcat(the_page,"</table><br/>\n");
    }  

    strcat(the_page, "Current Portmap Rules<br/><table border=\"1px\" cellpadding=\"3px\">\n");

    if (rulecount > 0) {
      for (int i = 0; i<IP_PORTMAP_MAX; i++) {
        if (portmap_tab[i].valid) {
            sprintf (buf,"<tr><td>%s", portmap_tab[i].proto == PROTO_TCP?"TCP ":"UDP ");
            strcat(the_page,buf);
            ip4_addr_t addr;
            addr.addr = my_ip;
            sprintf (buf, IPSTR":%d -> ", IP2STR(&addr), portmap_tab[i].mport);
            strcat(the_page,buf);
            addr.addr = portmap_tab[i].daddr;
            sprintf (buf, IPSTR":%d</td>\n", IP2STR(&addr), portmap_tab[i].dport);
            strcat(the_page,buf);
            sprintf (buf, "<td><input type=\"button\" value=\"Delete\" onclick=\"do_delete('PROTO_%s',%d);\"></td></tr>\n", portmap_tab[i].proto == PROTO_TCP?"TCP":"UDP",portmap_tab[i].mport);
            strcat(the_page,buf);
        }
      } 
    }
      else {
        sprintf (buf,"<tr><td>No rules set at this time</td></tr>\n");
        strcat(the_page,buf);
    }
    strcat(the_page, "</table>\n");

    // end portmap rules - start other items    
    strcat(the_page, "<br/><hr width=\"200px\"/><br/>\n");

    strcat(the_page, "Open Update Firmware page <input type=\"button\" value=\"Update Firmware\" onclick=\"do_updatefirmware();\"><br/><br/>\n");

    strcat(the_page, "Reboot ESP32 Local Port Forwarder <input type=\"button\" value=\"Reboot\" onclick=\"do_reboot();\"><br/><br/>\n");

    strcat(the_page, "Erase all settings and start over <input type=\"button\" value=\"Factory Reset\" onclick=\"do_factoryreset();\"><br/>\n");
    
    httpd_resp_send_chunk(req, the_page, strlen(the_page));
  } else { 

  const char msg3[] = " \n\
Set local network values for the ESP32 Local Port Forwarder<br/><br/> \n\
<form autocomplete=\"off\"> \n\
<table> \n\
<tr><td>SSID</td><td><input type='text' id='ssid' name='ssid' value='%s' placeholder='SSID of existing network'/></td></tr> \n\
<tr><td>SSID Password</td><td><input type='text' id='ssidpw' name='ssidpw' value='%s' placeholder='Password of existing network'/></td></tr> \n\
<tr><td>Static IP</td><td><input type='text' id='staticip' name='staticip' value='%s' placeholder='Local Static IP'/></td></tr> \n\
<tr><td>Subnet Mask</td><td><input type='text' id='subnetmask' name='subnetmask' value='%s'/></td></tr> \n\
<tr><td>Gateway IP</td><td><input type='text' id='gateway' name='gateway' value='%s'/></td></tr> \n\
<tr><td>OTA FW Updt PW</td><td><input type='text' id='otapw' name='otapw' value='%s'/></td></tr> \n\
<tr><td colspan='2' align='center'><input type='button' value='Connect' onclick=\"do_netconfig();\"/></td></tr> \n\
</table> \n\
</form> \n\
<br/>All fields must have values<br/><br/> \n\
If the values entered are valid, this AP address will be turned off and <br/> \n\
the ESP Local Port Forwarder site will be at the new static IP address. <br/> \n\
If the values are not valid, look for this network config site at %s. \n\
</div> \n\
</center> \n\
</body> \n\
</html> \n\
";

    sprintf(the_page, msg3, ssid, "", static_ip, subnet_mask, gateway_addr, ota_pw, DEFAULT_AP_IP);

    httpd_resp_send_chunk(req, the_page, strlen(the_page));
  }

  // send last chunks
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

static esp_err_t updatefirmware_get_handler(httpd_req_t *req) {

  //printf("In updatefirmware_get_handler\n");

    //const esp_partition_t *running = esp_ota_get_running_partition();
    //printf("Running partition type %d subtype %d (offset 0x%08x)",
    //         running->type, running->subtype, running->address);
    // type 0 is APP, 1 is DATA
    // subtype 16 is OTA_0, 17 is OTA_1

  const char msg[] = "<!doctype html> \n\
<html> \n\
<head> \n\
<meta charset=\"utf-8\"> \n\
<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> \n\
<title>ESP32 Firmware Updater</title> \n\
<script> \n\
 \n\
function clear_status() { \n\
  document.getElementById('status').innerHTML = \" &nbsp; \"; \n\
} \n\
 \n\
function uploadfile(file) { \n\
  if ( !document.getElementById(\"updatefile\").value ) { \n\
    alert( \"Choose a valid firmware file\" ); \n\
  } else { \n\
    let xhr = new XMLHttpRequest(); \n\
    document.getElementById('updatebutton').disabled = true; \n\
    document.getElementById('status').innerText = \"Progress 0%%\"; \n\
    let eraseNVSvalue = document.getElementById('EraseNVS').checked; \n\
    let upwd = document.getElementById('upwd').value; \n\
    // track upload progress \n\
    xhr.upload.onprogress = function(event) { \n\
      if (event.lengthComputable) { \n\
        var per = event.loaded / event.total; \n\
        document.getElementById('status').innerText = \"Progress \" + Math.round(per*100) + \"%%\"; \n\
      } \n\
    }; \n\
    // track completion: both successful or not \n\
    xhr.onloadend = function() { \n\
      if (xhr.status == 200) { \n\
        document.getElementById('status').innerText = xhr.response; \n\
      } else { \n\
        document.getElementById('status').innerText = \"Firmware update failed\"; \n\
      } \n\
      document.getElementById('updatebutton').disabled = false; \n\
      document.getElementById('upwd').value = \"\"; \n\
    }; \n\
    xhr.open(\"POST\", \"/updatefirmware\"); \n\
    xhr.setRequestHeader('EraseNVS', eraseNVSvalue); \n\
    xhr.setRequestHeader('UPwd', upwd); \n\
    xhr.send(file); \n\
  } \n\
} \n\
 \n\
</script> \n\
</head> \n\
<body><center> \n\
<h1>ESP32 Firmware Updater</h1> \n\
 \n\
Select an ESP32 firmware file (.bin) to update the ESP32 firmware<br><br> \n\
 \n\
<table> \n\
<tr><td align=\"center\"><input type=\"file\" id=\"updatefile\" accept=\".bin\" onclick=\"clear_status();\"><br><br></td></tr> \n\
<tr><td align=\"center\"><input type=\"checkbox\" id=\"EraseNVS\" onclick=\"clear_status();\"> Erase NVS<br><br></td></tr> \n\
<tr><td align=\"center\">Update Password <input type=\"password\" id=\"upwd\" maxlength=\"20\"><br><br></td></tr> \n\
<tr><td align=\"center\"><input type=\"button\" id=\"updatebutton\" onclick=\"uploadfile(updatefile.files[0]);\" value=\"Update\"><br><br></td></tr> \n\
<tr><td align=\"center\"><div id=\"status\"> &nbsp; </div><br><br></td></tr> \n\
<tr><td align=\"center\">%s Version %s</td></tr> \n\
</table> \n\
</center></body> \n\
</html>"; 

//  the_page = malloc(strlen(msg)+512);
  sprintf(the_page, msg, appName, appVersion);

  httpd_resp_send(req, the_page, strlen(the_page));

  return ESP_OK;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
#define UPLOAD_CACHE_SIZE 1600

static esp_err_t updatefirmware_post_handler(httpd_req_t *req) {

  //printf("In updatefirmware_post_handler\n");

  char contentBuffer[UPLOAD_CACHE_SIZE];
  size_t recv_size = sizeof(contentBuffer);
  size_t contentLen = req->content_len;

  char eraseNVS[10];
  httpd_req_get_hdr_value_str(req, "EraseNVS", eraseNVS, sizeof(eraseNVS));
  //printf("EraseNVS %s\n", eraseNVS );
  char upwd[20];
  httpd_req_get_hdr_value_str(req, "UPwd", upwd, sizeof(upwd));
  //printf("Update password %s\n", upwd);

  //printf("Content length is %d\n", contentLen);

  if ( !strcmp( ota_pw, upwd ) ) {
    // update password is good, do the firmware update
    //printf("in do the firmware update\n");

    // update handle : set by esp_ota_begin(), must be freed via esp_ota_end()
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    update_partition = esp_ota_get_next_update_partition(NULL);
    //printf("Writing to partition subtype %d (offset 0x%08x)",
    //         update_partition->subtype, update_partition->address);

    if (esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle) != ESP_OK) { //start flash with max available size
      strcpy(the_page, "Firmware update failed - Update failed begin step");
      printf("%s\n", the_page);
      httpd_resp_send(req, the_page, strlen(the_page));
      return ESP_OK;
    }
      
    size_t bytes_recvd = 0;
    while (bytes_recvd < contentLen) {
      int ret = httpd_req_recv(req, contentBuffer, recv_size);
      if (ret <= ESP_OK) {  // ESP_OK return value indicates connection closed 
        // Check if timeout occurred 
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
          httpd_resp_send_408(req);
        }
        return ESP_FAIL;
      }

      if (esp_ota_write(update_handle, (const void *) contentBuffer, ret) != ESP_OK) {
        strcpy(the_page, "Firmware update failed - Update failed write step");
        printf("%s\n", the_page);
        httpd_resp_send(req, the_page, strlen(the_page));
        return ESP_OK;
      }

      bytes_recvd += ret;
    }

    if (esp_ota_end(update_handle) != ESP_OK) { // end ota writing
      strcpy(the_page, "Firmware update failed - Update failed end step");
      printf("%s\n", the_page);
      httpd_resp_send(req, the_page, strlen(the_page));
      return ESP_OK;
    }

    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) { // set new boot partition
      strcpy(the_page, "Firmware update failed - Update failed set boot partition step");
      printf("%s\n", the_page);
      httpd_resp_send(req, the_page, strlen(the_page));
      return ESP_OK;
    }

    if ( !strcmp( "true", eraseNVS ) ) { // erase NVS
      printf("in erase EEPROM\n");

      nvs_handle_t nvs;
      esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
      if (err == ESP_OK) {
        err = nvs_erase_all(nvs);
        if (err == ESP_OK) {
            err = nvs_commit(nvs);
        }
      }
      ESP_LOGI(TAG, "Namespace '%s' was %s erased", PARAM_NAMESPACE, (err == ESP_OK) ? "" : "not");
      nvs_close(nvs);

      strcpy(the_page, "Firmware update and NVS erase successful - Rebooting");
    } else {
      strcpy(the_page, "Firmware update successful - Rebooting");
    }
    printf("%s\n", the_page);
    httpd_resp_send(req, the_page, strlen(the_page));

    delay(5000);
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP2S * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  } else {
    strcpy(the_page, "Firmware update failed - Invalid password");
  }

  printf("%s\n", the_page);
  httpd_resp_send(req, the_page, strlen(the_page));
  return ESP_OK;
}


static httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
};

httpd_uri_t updatefirmware_get_uri = {
    .uri       = "/updatefirmware",
    .method    = HTTP_GET,
    .handler   = updatefirmware_get_handler,
    .user_ctx  = NULL
};

  httpd_uri_t updatefirmware_post_uri = {
    .uri       = "/updatefirmware",
    .method    = HTTP_POST,
    .handler   = updatefirmware_post_handler,
    .user_ctx  = NULL
  };

httpd_handle_t start_webserver(void)
{
    the_page = malloc(5000);

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

//    const char *config_page_template = CONFIG_PAGE;
//    char *config_page = malloc(strlen(config_page_template)+512);
//    sprintf(config_page, config_page_template, ap_ssid, ap_passwd, ssid, passwd,
//            static_ip, subnet_mask, gateway_addr);
//    indexp.user_ctx = config_page;

    esp_timer_create(&restart_timer_args, &restart_timer);

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &updatefirmware_get_uri);
        httpd_register_uri_handler(server, &updatefirmware_post_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);

    free(the_page);
    the_page = NULL;

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 

//void startWebServer() {

  // start a web server for OTA updates
  // this same web server can be used for all other normal web server stuff
  // just add the appropriate uri handlers
  
//  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
//  config.server_port = SERVER_PORT;
//  config.stack_size = 8192;

//  httpd_uri_t updatefirmware_get_uri = {
//    .uri       = "/updatefirmware",
//    .method    = HTTP_GET,
//    .handler   = updatefirmware_get_handler,
//    .user_ctx  = NULL
//  };

//  httpd_uri_t updatefirmware_post_uri = {
//    .uri       = "/updatefirmware",
//    .method    = HTTP_POST,
//    .handler   = updatefirmware_post_handler,
//    .user_ctx  = NULL
//  };

//  if (httpd_start(&webserver_httpd, &config) == ESP_OK) {
//    httpd_register_uri_handler(webserver_httpd, &updatefirmware_get_uri);
//    httpd_register_uri_handler(webserver_httpd, &updatefirmware_post_uri);
//  }
//}


