package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"strings"
)

const (
	PORT = 8080
)

var hashList = []string{}

func isHashInList(hash string) bool {
	for _, h := range hashList {
		if h == hash {
			return true
		}
	}
	return false
}

func handleConnection(client net.Conn) {
	hash, _ := bufio.NewReader(client).ReadString('\n')

	// Trim the newline character from the message
	hash = hash[:len(hash)-1]

	fmt.Println("Message Received:", hash)

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
	fmt.Println("- Driver Verification Server -")
	fmt.Println("")

	hashList = ScanFile("hashes.txt")

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

func ScanFile(path string) []string {

	// support for unix systems
	// unix systems paste file paths with apostrophe
	if strings.Contains(path, "'") {
		path = strings.Trim(path, "'")
	}

	// open the file
	f, err := os.Open(path)

	// error handling
	if err != nil {
		log.Fatal(err)
	}

	defer f.Close()

	scanner := bufio.NewScanner(f)

	var lines []string

	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}

	// error handling
	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}

	return lines
}
