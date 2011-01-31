/*
 * testBlockingUDPClnt.cpp
 *
 *  Created on: Dec 28, 2010
 *      Author: Miha Vitorovic
 */

#include <remote.h>
#include <blockingUDP.h>
#include <logger.h>
#include <inetAddressUtil.h>

//#include <showConstructDestruct.h>

#include <osiSock.h>

#include <iostream>
#include <cstdio>

#define SRV_IP "127.0.0.1"

using namespace epics::pvAccess;
using namespace epics::pvData;

using std::cout;
using std::endl;
using std::sscanf;

static osiSockAddr sendTo;

class ContextImpl : public Context {
public:
    ContextImpl() 
    {}

    virtual ~ContextImpl() {
    }
    virtual Timer* getTimer() {
        return 0;
    }
    virtual TransportRegistry* getTransportRegistry() {
        return 0;
    }
    virtual Channel* getChannel(epics::pvAccess::pvAccessID) {
        return 0;
    }
    virtual Transport* getSearchTransport() {
        return 0;
    }
    virtual Configuration* getConfiguration() {
        return 0;
    }
};

class DummyResponseHandler : public ResponseHandler {
public:
    DummyResponseHandler(Context* ctx)
    { }

    virtual ~DummyResponseHandler() {}

    virtual void handleResponse(osiSockAddr* responseFrom,
            Transport* transport, int8 version, int8 command, int payloadSize,
            ByteBuffer* payloadBuffer) {
    }
};

class DummyTransportSender : public TransportSender {
public:
    DummyTransportSender() {
        for(int i = 0; i<20; i++)
            data[i] = (char)(i+1);
        count = 0;
    }

    virtual void send(ByteBuffer* buffer, TransportSendControl* control) {
        control->setRecipient(sendTo);

        // send the packet
        count++;
        control->startMessage((int8)(count+0x10), 0);
        buffer->put(data, 0, count);
        //control->endMessage();
    }

    virtual void lock() {
    }
    virtual void unlock() {
    }
    virtual void acquire() {
    }
    virtual void release() {
    }
private:
    char data[20];
    int count;
};

void testBlockingUDPSender() {
    BlockingUDPConnector connector(false, true);
    ContextImpl ctx;

    DummyTransportSender dts;
    DummyResponseHandler drh(&ctx);

    osiSockAddr bindAddr;

    bindAddr.ia.sin_family = AF_INET;
    bindAddr.ia.sin_port = htons(65001);
    bindAddr.ia.sin_addr.s_addr = htonl(INADDR_ANY);

    Transport* transport = connector.connect(NULL, &drh, bindAddr, 1, 50);

    // SRV_IP defined at the top of the this file
    if(aToIPAddr(SRV_IP, 65000, &sendTo.ia)<0) {
        cout<<"error in aToIPAddr(...)"<<endl;
        return;
    }

    cout<<"Sending 10 packets..."<<endl;

    for(int i = 0; i<10; i++) {
        cout<<"   Packet: "<<i+1<<endl;
        transport->enqueueSendRequest(&dts);
        sleep(1);
    }

    delete transport;

}

int main(int argc, char *argv[]) {
//    createFileLogger("testBlockingUDPClnt.log");

    testBlockingUDPSender();

//    std::cout << "-----------------------------------------------------------------------" << std::endl;
//    getShowConstructDestruct()->constuctDestructTotals(stdout);
    return (0);
}
