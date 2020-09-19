import requests
import json
from simplePublisher import *
import time


def getBroker(catalogUrl):
    return json.loads(requests.get(catalogUrl + "broker/").text)


def getDevices(catalogUrl):
    return json.loads(requests.get(catalogUrl + "devices/").text)


def registerService(catalog, client, topic, name):
    t = {}
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
    resource = str(input("Inserire risorsa"))

    if resource == "led":
        topic = devices[id]["end-points"]["mqtt-topics"][resource]
        broker_addr = broker["address"]
        broker_port = broker["port"]
        registerService(catalog, client, topic, resource)
        pub = MyPublisher(client, topic, broker)
        pub.start()
        while True:
            t = {"bn": "Yun", "e": [
                {"n": "led", "t": time.time(), "u": None, "v": "0"}]}
            status = str(input("Accendi/spegni led --> 1/0: "))

            t["e"][0]["v"] = status
            message = json.dumps(t)
            pub.myPublish(topic, message)
            time.sleep(10)
