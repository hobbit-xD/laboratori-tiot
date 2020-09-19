#!/usr/bin/python3
import paho.mqtt.client as PahoMQTT
from time import sleep, time
import json
import requests
from threading import Thread
from numpy import interp
import traceback
CATALOG_ADDRESS = "http://127.0.0.1:8080"

class SmartHomeController(object):
    
    def __init__(self):
        r = requests.get(CATALOG_ADDRESS + "/MQTT/broker").json()
        self.broker = r["address"]
        self.broker_port = r["port"]
        self.topics = []
        self.service = {}
        self.service["description"] = "Smart home controller"
        self.service["end-points"] = {"rest":[], "mqtt-topics":[]}

        self.presence = False
        self.running = False

        self.clientID = "SmartHomeController"
        self._paho_mqtt = PahoMQTT.Client(self.clientID, False) 

        self._paho_mqtt.on_connect = self.onConnect
        self._paho_mqtt.on_message = self.messageHandler

        self.registerThread = Thread(target=self.register)
        self.temps = {
            "heater":{"pmin":18, "pmax":24, "npmin":15, "npmax": 20},
            "fan":{"pmin":18, "pmax":24, "npmin":25, "npmax": 30}
            }

    def sendmessage(self):
        print("Inserisci il messaggio da inviare:")
        msg = input()
        for t in self.topics:
            if "/message" in t:
                self.publish(t, msg)

    def register(self):
        while(self.running):
            #print("Refreshing")
            requests.post(CATALOG_ADDRESS + "/services", json=self.service)
            sleep(5)
    
    def getTopics(self):
        r = requests.get(CATALOG_ADDRESS + "/devices").json()
        for device in r.values():
            if device["bn"] == "Yun":
                self.topics = device["end-points"]["mqtt-topics"]
                break
    
    def start(self):
        self._paho_mqtt.connect(self.broker, self.broker_port)
        self._paho_mqtt.loop_start()

        self.getTopics()

        for t in self.topics:
            if "data" in t:
                print("Subscribed to:", t)
                self._paho_mqtt.subscribe(t, 2)
                break
    
    def stop (self):
        for t in self.topics:
            if "data" in t:
                self._paho_mqtt.unsubscribe(t)
                break
        self.running = False
        self._paho_mqtt.loop_stop()
        self._paho_mqtt.disconnect()
    
    def publish(self, topic, message):
        self._paho_mqtt.publish(topic, message, 2)
    
    def messageHandler(self, paho_mqtt , userdata, msg):
        data = json.loads(msg.payload)
        try:
            if data['e'][0]['n'] == "temperature":
                self.handleTemp(data['e'][0]['v'])
            elif data['e'][0]['n'] == "presence":
                self.handlePresence(data['e'][0]['v'])
        except:
            pass
    
    def onConnect (self, paho_mqtt, userdata, flags, rc):
        print("Connected to %s with result code: %d" % (self.broker, rc))
        self.running = True
        self.registerThread.run()

    def handlePresence(self, value):
        if value == 0:
            self.presence = False
        elif value == 1:
            self.presence = True
    
    def setpoints(self):
        try:
            print("Inserisci soglia di azionamento della ventola in caso di presenza (gradi Cesius):")
            f_pmax = float(input())
            print("Inserisci soglia di spegnimento della ventola in caso di presenza (gradi Cesius):")
            f_pmin = float(input())
            print("Inserisci soglia di azionamento della ventola in caso di assenza (gradi Cesius):")
            f_npmax = float(input())
            print("Inserisci soglia di spegnimento della ventola in caso di assenza (gradi Cesius):")
            f_npmin = float(input())

            print("Inserisci soglia di azionamento del termistore in caso di presenza (gradi Cesius):")
            h_pmin = float(input())
            print("Inserisci soglia di spegnimento del termistore in caso di presenza (gradi Cesius):")
            h_pmax = float(input())
            print("Inserisci soglia di azionamento del termistore in caso di assenza (gradi Cesius):")
            h_npmin = float(input())
            print("Inserisci soglia di spegnimento del termistore in caso di assenza (gradi Cesius):")
            h_npmax = float(input())

            assert f_pmin < f_pmax
            assert f_npmin < f_npmax
            assert h_pmin < h_pmax
            assert h_npmin < h_npmax
        except:
            print("Qualcosa è andato storto, inserimento annullato")
        
        self.temps["fan"]["pmin"] = f_pmin
        self.temps["fan"]["pmax"] = f_pmax
        self.temps["fan"]["npmin"] = f_npmin
        self.temps["fan"]["npmax"] = f_npmax
        self.temps["heater"]["pmin"] = h_pmin
        self.temps["heater"]["pmax"] = h_pmax
        self.temps["heater"]["npmin"] = h_npmin
        self.temps["heater"]["npmax"] = h_npmax
    
    def userInteractor(self):
        print("Vuoi cambiare i setpoints? (yes/no)")
        c = input()
        if c == "yes":
            self.setpoints()
            
        print("Vuoi inviare un messaggio? (yes/no)")
        c = input()
        if c == "yes":
            self.sendmessage()

    def handleTemp(self, value):

        if self.presence:
            n_fan = int(interp(value, [ self.temps["fan"]["pmin"], self.temps["fan"]["pmax"] ], [0, 255]))
            #print("Fan: interp(", value, ", [", self.temps["fan"]["pmin"], self.temps["fan"]["pmax"], "], [0,255])")
            #print("Result:", n_fan)
            n_heater = int(interp(value, [self.temps["heater"]["pmin"], self.temps["heater"]["pmax"] ], [255, 0]))
        else:
            n_fan = int(interp(value, [ self.temps["fan"]["npmin"], self.temps["fan"]["npmax"]], [0, 255]))
            n_heater = int(interp(value, [self.temps["heater"]["npmin"], self.temps["heater"]["npmax"]], [255, 0]))

        hdata = {}
        hdata["bn"] = "shcc"
        hdata["e"] = [{"n":"heater","t":int(time()),"v":n_heater,"u":None}]

        fdata = {}
        fdata["bn"] = "shcc"
        fdata["e"] = [{"n":"fan","t":int(time()),"v":n_fan,"u":None}]

        for t in self.topics:
            if "/control" in t:
                self.publish(t, json.dumps(hdata))
                self.publish(t, json.dumps(fdata))



if __name__ == "__main__":
    shcc = SmartHomeController()
    shcc.start()
    while(not shcc.running):
        pass

    try:
        while(True):
            shcc.userInteractor()
    except:
        pass
    finally:
        print("Terminating client")
        shcc.stop()