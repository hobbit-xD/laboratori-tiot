import cherrypy
import json

class TemperatureClient():
    exposed = True

    def fToC(self,t):
        if t < -459.67:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return (t-32) * 5.0/9.0

    def fToK(self,t):
        if t < -459.67:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return (t-32) * 5.0/9.0 + 273.15

    def cToF(self,t):
        if t < -273.15:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return (t * 9.0/5.0) + 32

    def cToK(self,t):
        if t < -273.15:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return t + 273.15

    def kToC(self,t):
        if t < 0:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return t - 273.15

    def kToF(self,t):
        if t < 0:
            raise ValueError("Valore minimo di temperatura superato")
        else:
            return (t - 273.15) * 9.0/5.0 + 32

    def GET(self, **params):

        tmpDict = {}
        try:
            if(len(params)!=3):
                raise SyntaxError("Wrong number of parameter")
            
            for key in params.keys():
                if not key in ("value", "originalUnit", "targetUnit"):
                    raise SyntaxError("Una delle chiavi e' scritta in modo sbagliato") 

            tmpDict['value'] = int(params['value'])
            tmpDict['originalUnit'] = params['originalUnit']
            tmpDict['targetUnit'] = params['targetUnit']
          
            if tmpDict['originalUnit'] == 'F' and tmpDict['targetUnit'] == 'C':
                result = self.fToC(tmpDict['value'])
            elif tmpDict['originalUnit'] == 'F' and tmpDict['targetUnit'] == 'K':
                result = self.fToK(tmpDict['value'])
            elif tmpDict['originalUnit'] == 'C' and tmpDict['targetUnit'] == 'F':
                result = self.cToF(tmpDict['value'])
            elif tmpDict['originalUnit'] == 'C' and tmpDict['targetUnit'] == 'K':
                result = self.cToK(tmpDict['value'])
            elif tmpDict['originalUnit'] == 'K' and tmpDict['targetUnit'] == 'C':
                result = self.kToC(tmpDict['value'])
            elif tmpDict['originalUnit'] == 'K' and tmpDict['targetUnit'] == 'F':
                result = self.kToF(tmpDict['value'])
            elif tmpDict['originalUnit'] == tmpDict['targetUnit']:
                result = tmpDict['value']
            else:
                raise SyntaxError("Unita' sbagliata")
            
            tmpDict['result'] = result

        except SyntaxError:
            raise cherrypy.HTTPError(400, "I valori da passare sono tre: value, targetUnit, originalUnit")

        return json.dumps(tmpDict)

if __name__ == "__main__":
    
    conf = {
        '/': {
            'request.dispatch': cherrypy.dispatch.MethodDispatcher(),
            'tools.sessions.on': True
        }
    }

    cherrypy.tree.mount(TemperatureClient(),'/converter',conf)
    cherrypy.engine.start()
    cherrypy.engine.block()