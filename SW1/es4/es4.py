#!/usr/bin/python3
import cherrypy
import os
import json
from datetime import datetime

class FreeboardDeployer():
    exposed = True

    def GET(self, *uri, **params):
        return open("freeboard/index.html")
    
    def POST(self, *uri, **params):
        if uri == ('saveDashboard',):
            test = cherrypy.request.params["json_string"]
            print(test)
            time = datetime.now()
            timestamp = str(time.year) + str(time.month).rjust(2, "0") + str(time.day).rjust(2, "0")
            timestamp += str(time.hour).rjust(2, "0") + str(time.minute).rjust(2, "0") + str(time.second).rjust(2, "0")
            print(timestamp)
            with open("./freeboard/dashboard/Dashboard_"+timestamp+".json", "w") as out:
                out.write(test)
                print("Data written!")
        else:
            raise cherrypy.HTTPError(404)


if __name__ == "__main__":
    conf={
        '/':{
            'request.dispatch':cherrypy.dispatch.MethodDispatcher(),
            'tools.staticdir.on': True,
            'tools.staticdir.root': os.path.abspath(os.getcwd()),
            'tools.staticdir.dir': 'freeboard/',
            'request.show_tracebacks': False,
        }
    }
    cherrypy.tree.mount(FreeboardDeployer(),'/',conf)
    cherrypy.engine.start()
    cherrypy.engine.block()