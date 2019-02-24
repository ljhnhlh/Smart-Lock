package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/kataras/iris"
)

type respData struct {
	Status string      `json:"status"`
	Msg    string      `json:"msg"`
	Record interface{} `json:"record"`
}

type record struct {
	Time string `json:"time"`
}

type warningRecord struct {
	Time    string `json:"time"`
	Content string `json:"content"` //报警原因
}

var lockState = -1
var records []record
var warningRecords []warningRecord
var onlineLock = make(map[string]*net.Conn)
var lockName string
var lockPassword = "123456"
var temperature = "000"

func main() {
	//TCP
	go newTCP()
	app := iris.Default()
	app.Get("/api/allUnlockingRecord", func(ctx iris.Context) {
		msg := respData{
			Status: "200",
			Msg:    "获取成功",
			Record: records}
		b, err := json.Marshal(msg)
		if err == nil {
			ctx.WriteString(string(b))
		}
	})
	app.Get("/api/LockState", func(ctx iris.Context) {
		var msg respData
		msg.Status = "200"
		log.Println("[电子锁状态]", lockState)
		if lockState == -1 {
			msg.Msg = "电子锁未连接服务器"
		} else if lockState == 0 {
			msg.Msg = "电子锁关闭，状态正常"
		} else if lockState == 1 {
			msg.Msg = "电子锁开启"
		} else {
			msg.Msg = "电子锁状态异常，请检查电子锁"
		}
		temp := strings.Split(temperature, "")
		msg.Msg += "\n当前气温：" + temp[0] + temp[1] + "." + temp[2]
		b, err := json.Marshal(msg)
		if err == nil {
			ctx.WriteString(string(b))
		}
	})
	app.Get("/api/WarningRecord", func(ctx iris.Context) {
		msg := respData{
			Status: "200",
			Msg:    "获取成功",
			Record: warningRecords}
		b, err := json.Marshal(msg)
		if err == nil {
			ctx.WriteString(string(b))
		}
	})
	app.Post("/api/Unlocking", func(ctx iris.Context) {
		password := ctx.FormValue("password")
		var msg respData
		msg.Status = "200"
		if password == lockPassword {
			fmt.Println(onlineLock)
			conn, exits := onlineLock[lockName]
			fmt.Println(exits)
			if !exits {
				msg.Msg = "UnlockFailed\n电子锁不在线"
			} else {
				_, err := (*conn).Write([]byte("password " + password + "\n"))
				if err != nil {
					msg.Msg = "UnlockFailed\n电子锁状态异常"
				} else {
					msg.Msg = "UnlockOK"
					timestamp := time.Now().Unix()
					tm := time.Unix(timestamp, 0)
					currentTime := tm.Format("2006-01-02 15:04:05")
					fmt.Println("[UnlockTime]", currentTime)
					newRecord := record{Time: currentTime}
					msg.Record = []record{newRecord}
					records = append(records, newRecord)
				}
			}
		} else {
			msg.Msg = "UnlockFailed"
		}
		b, err := json.Marshal(msg)
		if err == nil {
			ctx.WriteString(string(b))
		}
	})
	// listen and serve on http://0.0.0.0:3000.
	app.Run(iris.Addr(":3000"))
}

func newTCP() {
	log.Println("Open TCP Server")
	service := ":3333"
	tcpAddr, err := net.ResolveTCPAddr("tcp4", service)
	checkError(err)
	log.Println("[TCP Address]", tcpAddr)
	listener, err := net.ListenTCP("tcp", tcpAddr)
	checkError(err)
	log.Println("[TCP Listen On]:", tcpAddr)
	for {
		conn, err := listener.Accept()
		if err != nil {
			continue
		}
		go handleConn(conn)
	}
}

/*
单片机TCP消息格式：
0. 0000-[电子锁名称] 电子锁连接FTP
1. 0001-[xx] 电子锁状态
2. 0002-[xx] 温度信息
3. 0003-[xx] 异常告警
4. 0004-[xx] 电子锁开锁成功
*/

func handleConn(conn net.Conn) {
	defer conn.Close() // close connection before exit
	defer log.Println(lockName, "Connect End")
	log.Println("New Connect")
	inputString, err := readInput(conn)
	checkError(err)
	log.Println("[Request]", inputString)
	temp := strings.SplitN(inputString, "-", 2)
	if len(temp) < 2 {
		lockName = "Undefine"
	} else {
		lockName = temp[1]
	}
	log.Println("[LockName]", lockName)
	onlineLock[lockName] = &conn
	lockState = 0
	defer delete(onlineLock, lockName)
	defer func() { lockState = -1 }()
	for {
		inputString, err := readInput(conn)
		if err != nil {
			log.Println(err)
			break
		}
		log.Println("[Request]", inputString)
		temp := strings.SplitN(inputString, "-", 2)
		if len(temp) < 2 {
			continue
		}
		stateCode := temp[0]
		message := temp[1]
		// fmt.Println("[Message]", message)
		switch stateCode {
		case "0001":
			var err error
			lockState, err = strconv.Atoi(message)
			checkError(err)
			break
		case "0002":
			temperature = message
			break
		case "0003":
			timestamp := time.Now().Unix()
			tm := time.Unix(timestamp, 0)
			currentTime := tm.Format("2006-01-02 15:04:05")
			log.Println("[Warning]", message)
			newRecord := warningRecord{Time: currentTime, Content: message}
			warningRecords = append(warningRecords, newRecord)
			lockState = 2
			break
		case "0004":
			timestamp := time.Now().Unix()
			tm := time.Unix(timestamp, 0)
			currentTime := tm.Format("2006-01-02 15:04:05")
			fmt.Println("[UnlockTime]", currentTime)
			newRecord := record{Time: currentTime}
			records = append(records, newRecord)
			break
		}
	}
}

func readInput(conn net.Conn) (string, error) {
	request := make([]byte, 256) // set maxium request length to 256B to prevent flood attack
	_, err := conn.Read(request)
	log.Println(string(request))
	return string(bytes.Trim(request, "\x00")), err
}

func checkError(err error) {
	if err != nil {
		fmt.Fprintf(os.Stderr, "Fatal error: %s", err.Error())
		os.Exit(1)
	}
}
