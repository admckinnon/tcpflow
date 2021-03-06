TCPFLOW 1.3
===========

By Simson L. Garfinkel <simsong@acm.org>
originally by Jeremy Elson <jelson@circlemud.org>

Downloads directory: http://www.digitalcorpora.org/downloads/tcpflow/


Compiling
---------
To compile for Linux

Download the sources with git, run bootstrap.sh, configure and make:

    git clone --recursive https://github.com/simsong/tcpflow.git
    cd tcpflow
    sh bootstrap.sh
    ./configure

To Compile for Windows with mingw on Fedora Core:


Be sure that mingw is installed, then 




Introduction
------------

tcpflow is a program that captures data transmitted as part of TCP
connections (flows), and stores the data in a way that is convenient
for protocol analysis and debugging.  Each TCP flow is stored in its
own file. Thus, the typical TCP flow will be stored in two files, one
for each direction. tcpflow can also process stored 'tcpdump' packet
flows.

tcpflow stores all captured data in files that have names of the form:

       [timestampT]sourceip.sourceport-destip.destport[--VLAN][cNNNN]

where:
  timestamp is an optional timestamp of the time that the first packet was seen
  T is a delimiter that indicates a timestamp was provided
  sourceip is the source IP address
  sourceport is the source port
  destip is the destination ip address
  destport is the destination port
  VLAN is the VLAN port
  c is a delimiter indicating that multiple connections are present
  NNNN is a connection counter, when there are multiple connections with 
      the same [time]/sourceip/sourceport/destip/destport combination.  
      Note that connection counting rarely happens when timestamp prefixing is performed.

HERE are some examples:

       128.129.130.131.02345-010.011.012.013.45103

  The contents of the above file would be data transmitted from
  host 128.129.131.131 port 2345, to host 10.11.12.13 port 45103.

       128.129.130.131.02345-010.011.012.013.45103c0005

  The sixth connection from 128.129.131.131 port 2345, to host 10.11.12.13 port 45103.

       1325542703T128.129.130.131.02345-010.011.012.013.45103

  A connection from 128.129.131.131 port 2345, to host 10.11.12.13 port 45103, that started on
  at 5:19pm (-0500) on January 2, 2012
  
       128.129.130.131.02345-010.011.012.013.45103--3

  A connection from 128.129.131.131 port 2345, to host 10.11.12.13
  port 45103 that was seen on VLAN port 3. 
   

You can change the template that is used to create filenames with the
-F and -T options.  If a directory appears in the template the directory will be automatically created.

If you use the -a option, tcpflow will automatically interpert HTTP responses.

       If the output file is
          208.111.153.175.00080-192.168.001.064.37314,

       Then the post-processing will create the files:
          208.111.153.175.00080-192.168.001.064.37314-HTTP
          208.111.153.175.00080-192.168.001.064.37314-HTTPBODY

       If the HTTPBODY was compressed with GZIP, you may get a 
       third file as well:

          208.111.153.175.00080-192.168.001.064.37314-HTTPBODY-GZIP

       Additional information about these streams, such as their MD5
       hash value, is also written to the DFXML file


tcpflow is similar to 'tcpdump', in that both process packets from the
wire or from a stored file. But it's different in that it reconstructs
the actual data streams and stores each flow in a separate file for
later analysis.

tcpflow understands sequence numbers and will correctly reconstruct
data streams regardless of retransmissions or out-of-order
delivery. However, tcpflow currently does not understand IP fragments; flows
containing IP fragments will not be recorded properly.

tcpflow can output a summary report file in DFXML format. This file
includes information about the systme on which the tcpflow program was
compiled, where it was run, and every TCP flow, including source and
destination IP addresses and ports, number of bytes, number of
packets, and (optionally) the MD5 hash of every bytestream. 

tcpflow uses the LBL Packet Capture Library (available at
ftp://ftp.ee.lbl.gov/libpcap.tar.Z) and therefore supports the same
rich filtering expressions that programs like 'tcpdump' support.  It
should compile under most popular versions of UNIX; see the INSTALL
file for details.

What use is it?
---------------

tcpflow is a useful tool for understanding network packet flows and
performing network forensics. Unlike programs such as WireShark, which
show lots of packets or a single TCP connection, tcpflow can show
hundreds, thousands, or hundreds of thousands of TCP connections in
context. 

A common use of tcpflow is to reveal the contents of HTTP
sessions. Using tcpflow you can reconstruct web pages downloaded over
HTTP. You can even extract malware delivered as 'drive-by downloads.'

Jeremy Elson originally wrote this program to capture the data being
sent by various programs that use undocumented network protocols in an
attempt to reverse engineer those protocols.  RealPlayer (and most
other streaming media players), ICQ, and AOL IM are good examples of
this type of application.  It was later used for HTTP protocol
analysis.

Simson Garfinkel founded Sandstorm Enterprises in 1998. Sandstorm
created a program similar to tcpflow called TCPDEMUX and another
version of the program called NetIntercept. Those programs are
commercial. After Simson left Sandstorm he had need for a tcp flow
reassembling program. He found tcpflow and took over its maintenance.

Bugs
----

Please enter bugs on the [github issue tracker](https://github.com/simsong/tcpflow/issues?state=open)

tcpflow currently does not understand IP fragments.  Flows containing
IP fragments will not be recorded correctly. IP fragmentation is
increasingly a rare event, so this does not seem to be a significant problem.


