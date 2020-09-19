#!/usr/bin/python3

import cherrypy
import json
import requests
import random, string

class CatalogClient(object):

    def __init__(self):
        self.host = "http://127.0.0.1:8080"
        self.errors = {
            200: "OK",
            400: "Bad Request",
            404: "Not Found",
            409: "Element already registered",
            500: "Internal server error"
        }

    def GETsomething(self, path, **devid):
        if devid == {}:
            r = requests.get(self.host + "/"+ path)
            print("GET: /"+path, r.json())
            #print("\n")
            #print(r)
        else:
            r = requests.get(self.host + "/"+ path, params=devid)
            try:
                print("GET: /"+path, r.json())
            except:
                print("GET: /"+path, r.status_code, self.errors[r.status_code])
    
    def POSTsomething(self, path, body):
        r = requests.post(self.host + "/"+ path, json=body)
        print("POST: /"+path, r.status_code, self.errors[r.status_code])
    
    def randomString(self, l):
        letters = string.ascii_lowercase
        return ''.join(random.choice(letters) for i in range(l))

    def createRandomUser(self):
        t = {}
        t["name"] = self.randomString(6)
        t["surname"] = self.randomString(6)
        t["email"] = []
        for i in range(random.randint(1,3)):
            t["email"].append(t["name"] + str(i) + "@email.it")
        return t
    
    def createRandomService(self):
        t = {}
        t["description"] = self.randomString(10)
        t["end-points"] = {"rest": [],"mqtt-topics": []}
        for _ in range(random.randint(0,2)):
            t["end-points"]["mqtt-topics"].append("/tiot/"+self.randomString(3))
        return t
    
    def createRandomDevice(self):
        t = {}
        t["end-points"] = {"rest": [],"mqtt-topics": []}
        t["resources"] = []
        for _ in range(random.randint(1,5)):
            t["resources"].append(self.randomString(5))
        for _ in range(random.randint(0,2)):
            t["end-points"]["mqtt-topics"].append("/tiot/"+self.randomString(3))
        return t


def queryHandler(c, method):
    
    paths = ["devices", "services", "users"]
    random = [c.createRandomDevice, c.createRandomService, c.createRandomUser]
    print("To which path?")
    print("0) /devices")
    print("1) /services")
    print("2) /users")
    inp = int(input())
    
    if inp not in [0,1,2]:
        print("Invalid command")
        return
    
    if method == "GET":
        print("Insert specific id (-1 to get all)")
        i = input()
        # id dei devices è ip, non un intero
        if inp != 0:
            i = int(i)
        if i == -1 or i == "-1":
            c.GETsomething(paths[inp])
        else:
            c.GETsomething(paths[inp], id=i)
    else:
        c.POSTsomething(paths[inp], random[inp]())



if __name__ == "__main__":
    c = CatalogClient()
    while True:
        print("RESTful Catalog Debug")
        print("0) POST data to Catalog")
        print("1) GET data from Catalog")
        print("2) Quit")
        
        inp = int(input())
    
        if inp == 0:
            queryHandler(c, "POST")
        elif inp == 1:
            queryHandler(c, "GET")
        elif inp == 2:
            print("Exiting")
            break
        else:
            print("Invalid command")