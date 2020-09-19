#!/usr/bin/python3

import cherrypy
import json
import requests
import time
import random
import string
import threading


class CatalogClient(threading.Thread):

    def __init__(self,id):
        self.id = id
        self.host = "http://127.0.0.1:8080"
        self.errors = {
            200: "OK",
            400: "Bad Request",
            404: "Not Found",
            409: "Element already registered",
            500: "Internal server error"
        }
        self.time = random.randint(4, 20)
        threading.Thread.__init__(self)

    def run(self):
        while True:
            random = self.createRandomDevice()
            self.POSTsomething("devices", random)
            time.sleep(self.time)

    def POSTsomething(self, path, body):
        print("Starting post")
        r = requests.post(self.host + "/" + path, json=body)
        print("POST: /"+path, r.status_code, self.errors[r.status_code])

    def randomString(self, l):
        letters = string.ascii_lowercase
        return ''.join(random.choice(letters) for i in range(l))

    def createRandomDevice(self):
        t = {}

        t["end-points"] = {"rest": [], "mqtt-topics": []}
        t["resources"] = []
        for _ in range(random.randint(1, 5)):
            t["resources"].append(self.randomString(5))
        for _ in range(random.randint(0, 2)):
            t["end-points"]["mqtt-topics"].append(
                "/tiot/"+self.randomString(3))
        return t


if __name__ == "__main__":
    threads = []

    for i in range(0, 5):
        threads.append(CatalogClient(i))

    for t in threads:
        t.start()
        time.sleep(60)
