#!/usr/bin/python3
import cherrypy
import json
import time
import os
from threading import Semaphore

class RESTfulCatalog(object):
    exposed = True

    def __init__(self):
        self.storage = os.path.join(os.path.dirname(__file__),"data.json")
        if os.path.isfile(self.storage):
            self.READdata()
        else:
            self.data = {}
            self.data["broker"] = {"address":"test.mosquitto.org", "port":1883}
            self.data["devices"] = {}
            self.data["services"] = {}
            self.data["users"] = {}
            # creazione del file vuoto
            self.WRITEdata()
        self.data_sem = Semaphore()
        cherrypy.process.plugins.Monitor(cherrypy.engine, self.CLEANdata, frequency=120).subscribe()

    def CLEANdata(self):
        self.ACQUIREsem("CLEANER")
        t = time.time()
        devices = 0
        keys = list(self.data["devices"].keys())
        for k in keys:
            if self.data["devices"][k]["insert-timestamp"] < t - 120:
                del self.data["devices"][k]
                devices += 1
        
        services = 0
        keys = list(self.data["services"].keys())
        for k in keys:
            if self.data["services"][k]["insert-timestamp"] < t - 120:
                del self.data["services"][k]
                services += 1
        self.WRITEdata()
        cherrypy.log(f"CLEANER: Deleted {devices} devices")
        cherrypy.log(f"CLEANER: Deleted {services} services")
        self.RELEASEsem("CLEANER")
            


    def WRITEdata(self):
        with open(self.storage, "w") as f:
            json.dump(self.data, f, indent=4)
    
    def READdata(self):
        with open(self.storage, "r") as f:
            self.data = json.load(f)

    def ACQUIREsem(self, who):
        self.data_sem.acquire()
        cherrypy.log(f"{who} acquired semaphore")
    
    def RELEASEsem(self, who):
        self.data_sem.release()
        cherrypy.log(f"{who} released semaphore")

    @cherrypy.tools.json_in()
    @cherrypy.tools.json_out()
    # viene utilizzato il metodo POST in quanto non si creano nuove posizioni nel path bensì si 
    # aggiungono record ad una posizione già esistente   
    def POST(self, *uri, **params):
        self.ACQUIREsem("POST")
        if uri == ('devices', ):
            t = self.POSTdevice(cherrypy.request)
        elif uri == ('users', ):
            t = self.POSTuser(cherrypy.request)
        elif uri == ('services', ):
            t = self.POSTservice(cherrypy.request)
        else:
            self.RELEASEsem("POST")
            raise cherrypy.HTTPError(404)
        
        self.RELEASEsem("POST")
        return t
    
    @cherrypy.tools.json_out()
    def GET(self, *uri, **params):
        self.ACQUIREsem("GET")
        if uri == ('broker',):
            t = self.GETall(uri[0])
        elif uri == ('devices',):
            if params == {}:
                t = self.GETall(uri[0])
            else:
                t = self.GETone(params, uri[0])
        elif uri == ('users',):
            if params == {}:
                t = self.GETall(uri[0])
            else:
                t = self.GETone(params, uri[0])
        elif uri == ('services',):
            if params == {}:
                t = self.GETall(uri[0])
            else:
                t = self.GETone(params, uri[0])
        else:
            self.RELEASEsem("GET")
            raise cherrypy.HTTPError(404)
        
        self.RELEASEsem("GET")
        return t

    def GETall(self, target):
        return self.data[target]
    
    def GETone(self, params, target):
        if "id" in params.keys():
            ID = str(params["id"])
            print("Id is:", ID)
            print(self.data[target])
            print(self.data[target].get(ID))
            try:
                return self.data[target][ID]
            except:
                self.RELEASEsem("GET")
                raise cherrypy.HTTPError(404, "Element with specified id not found.")
        else:
            self.RELEASEsem("GET")
            raise cherrypy.HTTPError(400, "You must specify an id.")
  
    def POSTdevice(self, req):
        try:
            obj = req.json
            ip = req.remote.ip
            # l'indirizzo ip del device viene utilizzato come identificativo, essendo univoco nella rete locale
            # in caso di cambio di indirizzo ip il device viene registrato nuovamente e il precedente viene eliminato
            # dopo i 2 minuti di timeout o sovrascritto da un nuovo device che ne ottiene l'ip
            device = {}
            assert isinstance(obj["resources"], list)
            assert isinstance(obj["end-points"], dict)
            assert isinstance(obj["end-points"]["rest"], list)
            assert isinstance(obj["end-points"]["mqtt-topics"], list)
            device["resources"] = obj["resources"]
            device["end-points"] = obj["end-points"]
            device["insert-timestamp"] = time.time()
            self.data["devices"][ip] = device

            self.WRITEdata()

            return self.data["devices"][ip]
        except:
            self.RELEASEsem("POST")
            raise cherrypy.HTTPError(400)

    def POSTuser(self, req):
        try:
            obj = req.json
            user = {}
            assert isinstance(obj["email"], list)
            user["name"] = obj["name"]
            user["surname"] = obj["surname"]
            obj["email"].sort()
            user["email"] = obj["email"]
        except:
            self.RELEASEsem("POST")
            raise cherrypy.HTTPError(400)
        
        for u in self.data["users"].values():
            for e in u["email"]:
                for new_e in user["email"]:
                    if new_e == e:
                        self.RELEASEsem("POST")
                        raise cherrypy.HTTPError(409, f"Email {new_e} already registered.")

        identifier = str(len(self.data["users"]))
        self.data["users"][identifier] = user

        self.WRITEdata()

        return self.data["users"][identifier]   

    def POSTservice(self, req):
        try:
            obj = req.json
            service = {}
            assert isinstance(obj["end-points"], dict)
            assert isinstance(obj["end-points"]["rest"], list)
            assert isinstance(obj["end-points"]["mqtt-topics"], list)
            service["description"] = obj["description"]
            obj["end-points"]["rest"].sort()
            obj["end-points"]["mqtt-topics"].sort()
            service["end-points"] = obj["end-points"]
        except:
            self.RELEASEsem("POST")
            raise cherrypy.HTTPError(400)
        
        refresh = False
        for k, s in self.data["services"].items():
            if service["description"] == s["description"] and service["end-points"] == s["end-points"]:
                refresh = True
                identifier = k
        
        if not refresh:
            identifiers = list(self.data["services"].keys())
            for i in range(len(identifiers)+1):
                if str(i) not in identifiers:
                    identifier = str(i)
                    break

        service["insert-timestamp"] = time.time()
        self.data["services"][identifier] = service

        self.WRITEdata()

        return self.data["services"][identifier]
        
if __name__ == "__main__":
    conf={
        '/':{
            'request.dispatch':cherrypy.dispatch.MethodDispatcher(),
            'request.show_tracebacks': True,
        }
    }
    cherrypy.tree.mount(RESTfulCatalog(),'/',conf)
    cherrypy.config.update({'server.socket_host': '0.0.0.0'})
    cherrypy.engine.start()
    cherrypy.engine.block()