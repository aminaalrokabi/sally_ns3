from __future__ import division
import sys
import os
import pylab

try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

def parse_time_ns(tm):
    if tm.endswith('ns'):
        return long(tm[:-4])
    raise ValueError(tm)



class FiveTuple(object):
    __slots__ = ['sourceAddress', 'destinationAddress', 'protocol', 'sourcePort', 'destinationPort']
    def __init__(self, el):
        self.sourceAddress = el.get('sourceAddress')
        self.destinationAddress = el.get('destinationAddress')
        self.sourcePort = int(el.get('sourcePort'))
        self.destinationPort = int(el.get('destinationPort'))
        self.protocol = int(el.get('protocol'))
        
class Histogram(object):
    __slots__ = 'bins', 'nbins', 'number_of_flows'
    def __init__(self, el=None):
        self.bins = []
        if el is not None:
            #self.nbins = int(el.get('nBins'))
            for bin in el.findall('bin'):
                self.bins.append( (float(bin.get("start")), float(bin.get("width")), int(bin.get("count"))) )

class Flow(object):

    def __init__(self, flow_el):
        self.flowId = int(flow_el.get('flowId'))
        rxPackets = long(flow_el.get('rxPackets'))
        txPackets = long(flow_el.get('txPackets'))
        tx_duration = float(long(flow_el.get('timeLastTxPacket')[:-4]) - long(flow_el.get('timeFirstTxPacket')[:-4]))*1e-9
        rx_duration = float(long(flow_el.get('timeLastRxPacket')[:-4]) - long(flow_el.get('timeFirstRxPacket')[:-4]))*1e-9
        self.rx_duration = rx_duration
        self.probe_stats_unsorted = []
            
        if rxPackets:
            self.hopCount = float(flow_el.get('timesForwarded')) / rxPackets + 1
            self.delayMean = float(flow_el.get('delaySum')[:-2]) / rxPackets * 1e-9
            self.jitterMean = float(flow_el.get('jitterSum')[:-2]) / rxPackets * 1e-9
            self.packetSizeMean = float(flow_el.get('rxBytes')) / rxPackets
            t0 = long(flow_el.get('timeFirstRxPacket')[:-4])
            t1 = long(flow_el.get('timeLastRxPacket')[:-4])
            duration=(t1-t0)*1e-9
            if duration:
                bitrates.append(8*long(flow_el.get('rxBytes'))/duration*1e-3)
                delays.append(float(flow_el.get('delaySum')[:-2])*1e-9/rxPackets)
                jitters.append(float(flow_el.get('jitterSum')[:-2])*1e-9/rxPackets)
            else:
                bitrates.append(0)
                delays.append(0)
                jitters.append(0)
    
        else:
            self.hopCount = -1000
            self.delayMean = None
            bitrates.append(0)
            delays.append(0)
            self.jitterMean = None
            self.packetSizeMean = None
            
        if rx_duration > 0:
            self.rxBitrate = long(flow_el.get('rxBytes'))*8 / rx_duration
        else:
            self.rxBitrate = None
        if tx_duration > 0:
            self.txBitrate = long(flow_el.get('txBytes'))*8 / tx_duration
        else:
            self.txBitrate = None

        lost = float(flow_el.get('lostPackets'))
        losses.append(lost)
        
        deliveries.append(rxPackets)
        if txPackets == 0:
            self.packetLossRatio = None
            self.packetDeliveryRatio = None
        else:
            self.packetLossRatio = ((lost * 100) / txPackets)
            self.packetDeliveryRatio = ((rxPackets * 100) / txPackets)

        interrupt_hist_elem = flow_el.find("flowInterruptionsHistogram")
        if interrupt_hist_elem is None:
            self.flowInterruptionsHistogram = None
        else:
            self.flowInterruptionsHistogram = Histogram(interrupt_hist_elem)


class ProbeFlowStats(object):
    __slots__ = ['probeId', 'packets', 'bytes', 'delayFromFirstProbe']

class Simulation(object):
    def __init__(self, simulation_el):
        self.flows = []
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        flow_map = {}
        
        for flow_el in simulation_el.findall("FlowStats/Flow"):
            flow = Flow(flow_el)
            flow_map[flow.flowId] = flow
            self.flows.append(flow)
        for flow_cls in FlowClassifier_el.findall("Flow"):
            flowId = int(flow_cls.get('flowId'))
            flow_map[flowId].fiveTuple = FiveTuple(flow_cls)

        for probe_elem in simulation_el.findall("FlowProbes/FlowProbe"):
            probeId = int(probe_elem.get('index'))
            for stats in probe_elem.findall("FlowStats"):
                flowId = int(stats.get('flowId'))
                s = ProbeFlowStats()
                s.packets = int(stats.get('packets'))
                s.bytes = long(stats.get('bytes'))
                s.probeId = probeId
                if s.packets > 0:
                    s.delayFromFirstProbe =  parse_time_ns(stats.get('delayFromFirstProbeSum')) / float(s.packets)
                else:
                    s.delayFromFirstProbe = 0
                flow_map[flowId].probe_stats_unsorted.append(s)


def main(argv):
    file_obj = open(argv[1])
    print "Reading XML file ",
 
    sys.stdout.flush()        
    level = 0
    sim_list = []
    global bitrates
    bitrates = []
    global losses
    losses = []
    global deliveries
    deliveries = []
    global delays
    delays = []
    global jitters
    jitters = []
        
    for event, elem in ElementTree.iterparse(file_obj, events=("start", "end")):
        if event == "start":
            level += 1
        if event == "end":
            level -= 1
            if level == 0 and elem.tag == 'FlowMonitor':
                sim = Simulation(elem)
                sim_list.append(sim)
                elem.clear() # won't need this any more
                sys.stdout.write(".")
                sys.stdout.flush()
    print " done."


    for sim in sim_list:
        for flow in sim.flows:
            t = flow.fiveTuple
            proto = {6: 'TCP', 17: 'UDP'} [t.protocol]
            print "FlowID: %i (%s %s/%s --> %s/%i)" % \
                (flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort)
            if (flow.txBitrate):
                print "\tTX bitrate: %.2f kbit/s" % (flow.txBitrate*1e-3,)
            else:
                print "\tTX bitrate: 0"
            if (flow.rxBitrate):
                print "\tRX bitrate: %.2f kbit/s" % (flow.rxBitrate*1e-3,)
            else:
                print "\tRX bitrate: 0"
            print "\tMean Delay: %.2f ms" % (flow.delayMean*1e3,)
            print "\tMean Jitter: %.2f ms" % (flow.jitterMean*1e3,)
            print "\tPacket Loss Ratio: %.2f %%" % (flow.packetLossRatio*100)
            print "\tPacket Delivery Ratio: %.2f %%" % (flow.packetDeliveryRatio*100)

    pylab.figure(figsize=(10, 15))
    
    
    pylab.subplot(511)
    n, bins, patches = pylab.hist(bitrates, bins=40)
    pylab.setp(patches, 'facecolor', 'g', 'alpha', 0.75)
    
    pylab.xlabel("Flow bitrate (bit/s")
    pylab.ylabel("Number of flows")
    
    pylab.subplot(512)
    n, bins, patches = pylab.hist(losses, bins=40)
    pylab.setp(patches, 'facecolor', 'r', 'alpha', 0.75)
    pylab.xlabel("Number of lost packets")
    pylab.ylabel("Number of flows")
    
    pylab.subplot(513)
    n, bins, patches = pylab.hist(deliveries, bins=40)
    pylab.setp(patches, 'facecolor', 'w', 'alpha', 0.75)
    pylab.xlabel("Number of delivered packets")
    pylab.ylabel("Number of flows")
    
    pylab.subplot(514)
    n, bins, patches = pylab.hist(delays, bins=20)
    pylab.setp(patches, 'facecolor', 'y', 'alpha', 0.75)
    pylab.xlabel("Delay  (s)")
    pylab.ylabel("Number of flows")
    
    pylab.subplot(515)
    n, bins, patches = pylab.hist(jitters, bins=20)
    pylab.setp(patches, 'facecolor', 'b', 'alpha', 0.75)
    pylab.xlabel("Jitter  (s)")
    pylab.ylabel("Number of flows")
    
    pylab.subplots_adjust(hspace=0.4)
    pylab.savefig("results.pdf")
    
if __name__ == '__main__':
    main(sys.argv)
