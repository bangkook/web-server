require 'socket'

starttime = Process.clock_gettime(Process::CLOCK_MONOTONIC)

s = TCPSocket.new 'localhost', 8989

s.write("GET /files/server_test2.txt HTTP/1.0\r\nHost: localhost\r\nAccept: text/html/png\r\n\r\nEmpty body for debugging\n")
s.write("GET /files/server_test2.txt HTTP/1.0\r\nHost: localhost\r\nAccept: text/html/png\r\n\r\nEmpty body for debugging\n")

s.close
endtime = Process.clock_gettime(Process::CLOCK_MONOTONIC)
elapsed = endtime - starttime
puts "#{elapsed}"