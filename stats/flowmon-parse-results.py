from __future__ import division
import sys
import os
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

def parse_time_ns(tm):
    if tm.endswith('ns'):
        return long(tm[:-4])
    raise ValueError(tm)

def getCustomStats(protocol, network_size):
    level = 0
    for event, elem in ElementTree.iterparse(open("%s.custom.%d" % (protocol, network_size)), events=("start", "end")):
        if event == "start":
            level += 1
        if event == "end":
            level -= 1
            if level == 0 and elem.tag == 'CustomStats':
                custom_stat_el = elem.find('RoutingStats')
                return (int(custom_stat_el.get('numPackets')), int(custom_stat_el.get('numControlPackets')))


class Flow(object):

    def __init__(self, flow_el):
        self.flowId = int(flow_el.get('flowId'))
        self.rxBytes = int(flow_el.get('rxBytes'))
        self.txBytes = int(flow_el.get('txBytes'))
        self.rxPackets = long(flow_el.get('rxPackets'))
        txPackets = long(flow_el.get('txPackets'))
        tx_duration = float(long(flow_el.get('timeLastTxPacket')[:-4]) - long(flow_el.get('timeFirstTxPacket')[:-4]))*1e-9
        rx_duration = float(long(flow_el.get('timeLastRxPacket')[:-4]) - long(flow_el.get('timeFirstRxPacket')[:-4]))*1e-9
        self.rx_duration = rx_duration
        self.probe_stats_unsorted = []
            
        if self.rxPackets:
            self.hopCount = float(flow_el.get('timesForwarded')) / self.rxPackets + 1
            self.delayMean = float(flow_el.get('delaySum')[:-2]) / self.rxPackets * 1e-9
            self.jitterMean = float(flow_el.get('jitterSum')[:-2]) / self.rxPackets * 1e-9
            self.packetSizeMean = float(flow_el.get('rxBytes')) / self.rxPackets
            self.lost = float(flow_el.get('lostPackets'))
            
        else:
            self.hopCount = -1000
            self.delayMean = None
            self.jitterMean = None
            self.packetSizeMean = None
            self.packetsDelivered = None
            self.lost = None
            
        if rx_duration > 0:
            self.rxBitrate = long(flow_el.get('rxBytes'))*8 / rx_duration
        else:
            self.rxBitrate = 0
        if tx_duration > 0:
            self.txBitrate = long(flow_el.get('txBytes'))*8 / tx_duration
        else:
            self.txBitrate = 0

        
        if self.rxPackets == 0:
            self.packetLossRatio = None
        else:
            self.packetLossRatio = (self.lost / txPackets)


class Simulation(object):
    def __init__(self, simulation_el):
        self.flows = []
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        flow_map = {}
        
        for flow_el in simulation_el.findall("FlowStats/Flow"):
            flow = Flow(flow_el)
            if flow.rxPackets:
                flow_map[flow.flowId] = flow
                self.flows.append(flow)
        
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        
        self.dataPackets = 0
        self.throughput = 0;
        for flow_cls in FlowClassifier_el.findall("Flow"):
            flowId = int(flow_cls.get('flowId'))
            port = int(flow_cls.get('destinationPort'))
            numDataFlows = 0
            if port == 9:
                try:
                    numDataFlows +=1  
                    self.dataPackets += flow_map[flowId].rxPackets
                    self.throughput += (flow_map[flowId].rxBytes / flow_map[flowId].txBytes) 
                except KeyError:
                    pass
        if self.throughput == 0:
            self.throughput = self.throughput / numDataFlows; 
              

def main(argv):
    protocols = ["SALLY", "OLSR", "AODV", "CHAINED"]
    network_sizes = [5,10,15,20,25,30,35,40,45,50]
    network_sizes = [5,10,15,20]
    colours = ['r','y','g','b']
    simulations = [] 
     
    level = 0
    sim_list = {}
    for protocol in protocols:
        sim_list[protocol] = []

    for protocol in protocols:
        for network_size in network_sizes: 
            for event, elem in ElementTree.iterparse(open("%s.flomonitor.%d" % (protocol, network_size)), events=("start", "end")):
                if event == "start":
                    level += 1
                if event == "end":
                    level -= 1
                    if level == 0 and elem.tag == 'FlowMonitor':
                        sim = Simulation(elem)
                        custom_stats = getCustomStats(protocol, network_size)
                        sim.numPackets = custom_stats[0]
                        sim.numControlPackets = custom_stats[1]
                        sim_list[protocol].append((sim, network_size))
                        elem.clear() # won't need this any more
            

    for sim_pair in sim_list.iteritems():
        protocol = sim_pair[0]
        sims = sim_pair[1]
        print "stats for %s" % protocol
        
        for sim_pair in sims:
            sim = sim_pair[0]
            network_size = sim_pair[1]
            print "Network size %d" % network_size
            print "\tTX bitrate: %.2f kbit/s" % (sum((flow.txBitrate*1e3) for flow in sim.flows)/len(sim.flows))
            print "\tRX bitrate: %.2f kbit/s" % (sum((flow.rxBitrate*1e3) for flow in sim.flows)/len(sim.flows))
            print "\tDelay mean: %.2f ms" % (sum((flow.delayMean*1e3) for flow in sim.flows)/len(sim.flows))
            print "\tJitter mean: %.2f ms" % (sum((flow.jitterMean*1e3) for flow in sim.flows)/len(sim.flows))
            print "\tPacket size mean: %.2f" % (sum(flow.packetSizeMean for flow in sim.flows)/len(sim.flows))
            print "\tLost packets %.2f" % (sum(flow.lost for flow in sim.flows)/len(sim.flows))
            print "\tPacket loss ratio: %.2f %%" % (sum((flow.packetLossRatio*100) for flow in sim.flows)/len(sim.flows))
            print "\tNormalised Routing overhead (%d total/%d data packets): %.2f" % ((sum((flow.rxPackets) for flow in sim.flows), sim.dataPackets, (sum((flow.rxPackets) for flow in sim.flows)/sim.dataPackets) * 64))
     
    N = len(network_sizes)
    ind = np.arange(N)
    width = 0.35
    
    f, axarr = plt.subplots(3,2)
    rects = []
    for i, protocol in enumerate(protocols):
        results = []
        for sim_pair in sim_list[protocol]:
            results.append(sim_pair[0].throughput)
        rects.append(axarr[0][0].bar((ind*(N*width)+(i*width)), results, width, color=colours[i]))
    axarr[0][0].set_ylabel('Throughput (kbit/s)')
    axarr[0][0].set_xlabel('Network size (nodes)')
    axarr[0][0].set_title('Throughput against network size')
    axarr[0][0].set_xticks(((np.arange(N)+(np.arange(N)+1))*(width*10*(N/2)))/10.0)
    axarr[0][0].set_xticklabels(tuple(network_sizes))
    
    axarr[0][0].legend(tuple([l[0] for l in rects]),tuple(protocols))
    
    rects = []
    for i, protocol in enumerate(protocols):
        results = []
        for sim_pair in sim_list[protocol]:
            results.append((sum((flow.delayMean*1e3) for flow in sim_pair[0].flows))/len(sim.flows))
        rects.append(axarr[0][1].bar((ind*(N*width)+(i*width)), results, width, color=colours[i]))
    axarr[0][1].set_ylabel('Delay (ms)')
    axarr[0][1].set_xlabel('Network size (nodes)')
    axarr[0][1].set_title('End to End delay against network size')
    axarr[0][1].set_xticks(((np.arange(N)+(np.arange(N)+1))*(width*10*(N/2)))/10.0)
    axarr[0][1].set_xticklabels(tuple(network_sizes))
    
    axarr[0][1].legend(tuple([l[0] for l in rects]),tuple(protocols))
    
    rects = []
    for i, protocol in enumerate(protocols):
        results = []
        for sim_pair in sim_list[protocol]:
            results.append((sum((flow.jitterMean*1e3) for flow in sim_pair[0].flows))/len(sim.flows))
        rects.append(axarr[1][0].bar((ind*(N*width)+(i*width)), results, width, color=colours[i]))
    axarr[1][0].set_ylabel('Jitter (ms)')
    axarr[1][0].set_xlabel('Network size (nodes)')
    axarr[1][0].set_title('Jitter against network size')
    axarr[1][0].set_xticks(((np.arange(N)+(np.arange(N)+1))*(width*10*(N/2)))/10.0)
    axarr[1][0].set_xticklabels(tuple(network_sizes))
    
    axarr[1][0].legend(tuple([l[0] for l in rects]),tuple(protocols))
    
    rects = []
    for i, protocol in enumerate(protocols):
        results = []
        for sim_pair in sim_list[protocol]:
            results.append((sum((flow.packetLossRatio*100) for flow in sim_pair[0].flows))/len(sim.flows))
        rects.append(axarr[1][1].bar((ind*(N*width)+(i*width)), results, width, color=colours[i]))
    axarr[1][1].set_ylabel('Packet loss (%)')
    axarr[1][1].set_xlabel('Network size (nodes)')
    axarr[1][1].set_title('Packet loss ratio against network size')
    axarr[1][1].set_xticks(((np.arange(N)+(np.arange(N)+1))*(width*10*(N/2)))/10.0)
    axarr[1][1].set_xticklabels(tuple(network_sizes))
    
    axarr[1][1].legend(tuple([l[0] for l in rects]),tuple(protocols))
    
    rects = []
    for i, protocol in enumerate(protocols):
        results = []
        for sim_pair in sim_list[protocol]:
            results.append(sim_pair[0].numControlPackets)
        rects.append(axarr[2][0].bar((ind*(N*width)+(i*width)), results, width, color=colours[i]))
    axarr[2][0].set_ylabel('Routing overhead')
    axarr[2][0].set_xlabel('Network size (nodes)')
    axarr[2][0].set_title('Normalised routing overhead against network size')
    axarr[2][0].set_xticks(((np.arange(N)+(np.arange(N)+1))*(width*10*(N/2)))/10.0)
    axarr[2][0].set_xticklabels(tuple(network_sizes))
    
    axarr[2][0].legend(tuple([l[0] for l in rects]),tuple(protocols))
    
    plt.show()
    
if __name__ == '__main__':
    main(sys.argv)

    
