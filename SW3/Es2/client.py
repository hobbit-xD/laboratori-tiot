import requests
import json
from simpleSubscriber import *

def getBroker(catalogUrl):
    return json.loads(requests.get(catalogUrl + "broker/").text)

def getDevices(catalogUrl):
    return json.loads(requests.get(catalogUrl + "devices/").text)

def registerService(catalog,client,topic,name):
    t ={}
    t['id'] = client
    t['description'] = "temperature retrieve"
    t['end-points']["mqtt-topics"][name] = topic
    requests.post(catalog + "services/", t)

if __name__ == "__main__":
    
    catalog = "http://0.0.0.0:8080/"
    client = "test"
    broker = getBroker(catalog)
    print(broker)

    devices = getDevices(catalog)
    for device in devices:
        print("id: {}".format(json.dumps(device)))

    trovato = False
    while not trovato:
        id = str(input("Inserisci id device: "))
        print(id)
        for device in devices:
            if str(device) == id:
                trovato = True
                break

    print(devices[id]["end-points"])
    tmp = str(input("Inserire risorsa"))
    topic = devices[id]["end-points"]["mqtt-topics"][tmp]
    broker_addr = broker["address"]
    broker_port = broker["port"]

    registerService(catalog,client,topic,tmp)
    sub = MySubscriber(client,topic,broker)
    sub.start()
    while True:
        pass