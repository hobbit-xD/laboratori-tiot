#!/usr/bin/python3
import cherrypy
import json

class TemperatureServer():
    exposed = True

    def __init__(self):
        self.temps = {}
        self.temps["list"] = []
    
    @cherrypy.tools.json_in()
    def POST(self, *uri, **params):
        if uri == ('log', ):
            json_temp = cherrypy.request.json
            try:
                keys = json_temp.keys()
                assert "bn" in keys and "e" in keys
                assert json_temp["bn"] == "Yun"
                
                keys = json_temp["e"][0].keys()
                assert "n" in keys and "t" in keys and "v" in keys and "u" in keys
                assert json_temp["e"][0]["n"] == "temperature" and json_temp["e"][0]["u"] == "Cel"
                assert json_temp["e"][0]["t"] != None and json_temp["e"][0]["v"] != None
            except:
                raise cherrypy.HTTPError(400)

            self.temps["list"].append(json_temp)
        else:
            raise cherrypy.HTTPError(404)

    @cherrypy.tools.json_out()
    def GET(self, *uri, **params):
        if uri == ('log', ):
            return self.temps
        else:
            raise cherrypy.HTTPError(404)
    

if __name__ == "__main__":
    conf={
        '/':{
            'request.dispatch':cherrypy.dispatch.MethodDispatcher(),
            'request.show_tracebacks': False,
        }
    }
    cherrypy.tree.mount(TemperatureServer(),'/',conf)
    cherrypy.config.update({'server.socket_host': '0.0.0.0'})
    cherrypy.engine.start()
    cherrypy.engine.block()