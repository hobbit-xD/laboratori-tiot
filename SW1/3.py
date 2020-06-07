#!/usr/bin/python3
import cherrypy


class TemperatureClient():
    exposed = True
    
    def celsius2kelvin(self, t):
        if t < -273.15:
            raise Exception
        return t + 273.15
    
    def kelvin2celsius(self, t):
        if t < 0:
            raise Exception
        return t - 273.15

    @cherrypy.tools.json_in()
    @cherrypy.tools.json_out()
    def PUT(self, *uri, **params):
        try:
            json_temps = cherrypy.request.json
            print(json_temps)
            values = json_temps["values"]
            originalUnit = json_temps["originalUnit"]
            targetUnit = json_temps["targetUnit"]

            if originalUnit == "C" and targetUnit == "K":
                targetFunc = self.celsius2kelvin
            elif originalUnit == "K" and targetUnit == "C":
                targetFunc = self.kelvin2celsius
            else:
                raise Exception
            
            targetValues = []
            for v in values:
                targetValues.append(targetFunc(v))
            result = json_temps
            result["targetValues"] = targetValues

            return result
            
        except:
            errorMsg = "Your request: {} ".format(json_temps)
            errorMsg += "Correct example: {'originalUnit': 'C', 'targetUnit': 'K', 'values': [1,2,3,4]} "
            errorMsg += "C or K are only units available."
            
            raise cherrypy.HTTPError(400, errorMsg)     

if __name__ == "__main__":
    conf={
        '/':{
            'request.dispatch':cherrypy.dispatch.MethodDispatcher(),
            'request.show_tracebacks': False,
        }
    }
    cherrypy.tree.mount(TemperatureClient(),'/temperaturePUT',conf)
    cherrypy.engine.start()
    cherrypy.engine.block()