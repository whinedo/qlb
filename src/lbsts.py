#!/usr/bin/python

import re
import sys



def isElemIn(type,elem,list):

	elems = len(list)

	if (elems == 0):
		return -1

	for i in range(elems):
		if (type in list[i].keys()) and (elem == list[i][type]): 
			return i

	return -1



serverRe = re.compile('(.+)(dst=)(\d+\.\d+\.\d+\.\d+)( .+)')
serviceRe = re.compile('(.+)(dport=)(\d+)( .+)')

establishedRe = re.compile('(.+)(dport=)(\d+)( .+)(src=)(\d+\.\d+\.\d+\.\d+)( .+)')

try:
	fd=open('/etc/lbdt.conf','r')

except Exception,e:
	print "Cannot open /etc/lbdt.conf. Reason:",e

lines = fd.readlines()

services = []
servers = []

for line in lines:
	

	try:
		(lbport,serverIp,serverPort) = line.split(' ',2)

	except Exception,e:
		print e
		continue

	if isElemIn('lbport',int(lbport),services) == -1:
		service = {'lbport' : int(lbport),
			   'connections' : 0}
		services.append(service)

	if isElemIn('ip',serverIp,servers) == -1:

		server = {'ip': serverIp,
			  'connections' : 0}

		servers.append(server)
#DEBUG
#print services
#print servers
#FINDEBUG


server = None
service = None

try:
	fd=open('/proc/net/ip_conntrack','r')

except Exception,e:
	print "Cannot open /proc/net/ip_conntrack. Reason:",e

lines = fd.readlines()

for line in lines:
	if "ESTABLISHED" in line:
	#	res = serverRe.search(line)
	#	if res != None:
	#		try:
	#			server = res.group(3)
	#		except:
	#			continue
	#		i = isElemIn('ip',server,servers)
	#		if i != -1:
	#			servers[i]['connections'] += 1
	#	else:
	#		res = serviceRe.search(line)
	#		if res != None:
	#			try:
	#				service = res.group(3)
	#			except:
	#				continue

	#			i = isElemIn('lbport',service,services)
	#			print service
	#			if i != -1:
	#				services[i]['connections'] += 1
		#			print service
			

		res = establishedRe.search(line)
		if res != None:
			try:
				service = res.group(3)
				server = res.group(6)
			except:
				continue

			i = isElemIn('ip',server,servers)
			#if i != -1:
			j = isElemIn('lbport',int(service),services)

			if (i != -1) and (j != -1): #both in the same line in ip_conntrack
				servers[i]['connections'] += 1
				services[j]['connections'] += 1




#print data

print "--------------"
print "Service\t\tConnections"
for service in services:
	print "%d\t\t%d"%(service['lbport'],service['connections'])

print "--------------"
print "Server \t\tConnections"

for server in servers:
	print "%s\t%d"%(server['ip'],server['connections'])

