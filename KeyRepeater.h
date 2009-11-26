#ifndef _RT_KEY_REPEATER_H_
#define _RT_KEY_REPEATER_H_

#include <Devices/IKeyboard.h>

using namespace OpenEngine::Devices;

class KeyRepeater : public IListener<KeyboardEventArg>, 
                    public IListener<ProcessEventArg> {
    
    struct DK {
        Key sym;        

        bool operator<(const DK d) const {
            return sym < d.sym;
        }

        DK(Key k) : sym(k) {}
    };

    map<DK,Timer> downKeys;
    
    //Timer timer;
    Event<KeyboardEventArg> ke;

public:
    KeyRepeater() {

    }

    void Handle(ProcessEventArg arg) {
        for (map<DK,Timer>::iterator itr = downKeys.begin();
             itr != downKeys.end();
             itr++) {
            DK d = itr->first;
            Timer t = itr->second;
            if (t.GetElapsedIntervals(500000)) {
            
                KeyboardEventArg a;
                a.sym = d.sym;
                a.type = EVENT_RELEASE;


                ke.Notify(a);

                   
                a.type = EVENT_PRESS;               
                ke.Notify(a);
            }
        }
    }

    void Handle(KeyboardEventArg arg) {
        if (arg.type == EVENT_PRESS) {
            Timer t;
            t.Start();
            downKeys.insert(pair<DK,Timer>(DK(arg.sym),t));
            ke.Notify(arg);
        } else {
            downKeys.erase(DK(arg.sym));
            ke.Notify(arg);
        }
    }

    IEvent<KeyboardEventArg>& KeyEvent() { return ke; }

};


#endif
