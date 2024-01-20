package main

import (
	"bufio"
	"fmt"
	"net"
	"strconv"
)

const (
	PORT = 8080
)

var hashList = []string{
	"-1221513979",
	"921297973",
}

func isHashInList(hash string) bool {
	for _, h := range hashList {
		if h == hash {
			return true
		}
	}
	return false
}

func handleConnection(client net.Conn) {
	// get message, output
	hash, _ := bufio.NewReader(client).ReadString('\n')

	// Trim the newline character from the message
	hash = hash[:len(hash)-1]

	fmt.Println("Message Received:", hash)

	// Check if the message is in the hash list
	if isHashInList(hash) {
		fmt.Println("Sending 'rejected' response to", client.RemoteAddr())
		client.Write([]byte("rejected\n"))
	} else {
		fmt.Println("Sending 'approved' response to", client.RemoteAddr())
		client.Write([]byte("approved\n"))
	}

	client.Close()
}

func main() {
	fmt.Println("<- Driver Verification Server ->")
	fmt.Println("")

	server, err := net.Listen("tcp", ":"+strconv.Itoa(PORT))
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	defer server.Close()

	fmt.Println("Listening for connections . . .")
	fmt.Println("")
	for {
		client, _ := server.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err)
			continue
		}
		fmt.Println("Accepted connection from:", client.RemoteAddr())

		// Handle the connection in a new goroutine
		go handleConnection(client)
	}
}