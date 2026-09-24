//
//  ContentView.swift
//  pppm
//
//  Created by Michael Scott on 06/09/2026.
//

import SwiftUI
import CryptoKit
internal import Combine
internal import Combine
internal import Combine

//globals
let iterations: Int = 20000
let pinLength: Int = 4

var notfirstuse: Bool = false

var randomSecret: String = ""
var masterSecret : String = ""
var chosenServer : String = "none"
var pinSecret = ""
var firstlabel : String = "Rand"
var prompt : String = ""
var user : String = "" // default username

var digestMaster : Data = Data()
var digestServer : Data = Data()
var digestPin : Data = Data()

struct Server {
    var username: String
    var service: String
    var domain: String
    var note: String
    var policy_len: Int
}

//database

var list:[Server] = []
var serverlist:[String] = []

func saveSites() {
    let directory=URL.documentsDirectory
    let siteURL=directory.appending(path: "sites.txt")
    
    var sitesContents=""
    for line in list {
        let csvOutput="\(line.username),\(line.service),\(line.domain),\(line.note),\(line.policy_len)\n"
        sitesContents.append(csvOutput)
    }
    do {
        let data=sitesContents.data(using: .utf8)
        try data?.write(to: siteURL)
    } catch {
        print("Error writing sites.txt")
    }
}

// start-up checks
func check() {
    let directory=URL.documentsDirectory
    //print("checking random file")
    print("directory :\(directory.path())")
    let fileURL=directory.appending(path : "rand.txt")

    notfirstuse = FileManager.default.fileExists(atPath: fileURL.path)
    if notfirstuse {
        firstlabel="Master"
        prompt="Enter Master Secret"
        //("master field")
        let directory=URL.documentsDirectory
        //print("directory :\(directory.path())")
        let fileURL=directory.appending(path : "rand.txt")
        let randcontents=try! String(contentsOf: fileURL)
        let randlines=randcontents.split(separator: "\n")
        randomSecret=String(randlines[0])
        user=String(randlines[1])
        
        let siteURL=directory.appending(path: "sites.txt")
        let siteContents=try! String(contentsOf: siteURL)
        let sitelines=siteContents.split(separator: "\n")
        for line in sitelines {
            let columns = line.components(separatedBy: ",")
            //print(columns.count)
            if columns.count==5 {
                let username=columns[0]
                let service=columns[1]
                let domain=columns[2]
                let note=columns[3]
                let len=Int(columns[4]) ?? 12
                let server=Server(username: username, service: service, domain: domain, note: note, policy_len: len)
                list.append(server)
                serverlist.append(server.service)
            }
        }
        // read in from sites.txt
    } else {
        firstlabel="Random"
        prompt="Enter some random words"
        print("random field")
        //userEnabled=true
    }
}

// create rand.txt file and initial sites.txt
func saveRandFile() {
    let directory=URL.documentsDirectory
    print("saving random file")
    print("directory :\(directory.path())")
    let fileURL=directory.appending(path : "rand.txt")
    do {
        let output=masterSecret+"\n"+user
        let data=output.data(using: .utf8)
        try data?.write(to: fileURL)
    } catch {
        print("Error writing")
    }
    
    let siteURL=directory.appending(path : "sites.txt")
    do {
        let output=",none,,,"
        // mcarrickscott:gmail.com,vhi,vhi.ie,mikes vhi,12
        let data=output.data(using: .utf8)
        try data?.write(to: siteURL)
    } catch {
        print("Error writing")
    }
}

struct ContentView: View {
    @State private var label = firstlabel
    @State private var master = ""
    @State private var username = ""
    @State private var website = ""
    @State private var note = ""
    @State private var pin = ""
    @State private var length = ""
    @State private var password = ""
    @State private var service = ""
    @State private var show = false
    //@State private var maxLength: Int = 4
    @State private var masterEntered : Bool = false
    
    @State private var selectedServer: String = "none"
    
    @State private var masterEnabled = true
    @State private var userEnabled = false
    @State private var webEnabled = false
    @State private var noteEnabled = false
    @State private var serviceEnabled = false
    @State private var pinEnabled = false

    @State private var newEnabled=true
    @State private var addEnabled=false
    @State private var deleteEnabled=false
    @State private var sureEnabled=false
    
    @State private var newPressed = false
    @State private var passwordPrompt = ""
    
    @FocusState private var masterFocus : Bool
    @FocusState private var serviceFocus: Bool
    @FocusState private var pinFocus: Bool
    
    func reset() {
        master=""; username=""; website=""; note=""; password=""; service=""; length=""; pin=""
        masterFocus=true
        serviceFocus=false
        pinFocus=false
        masterSecret=""
        masterEntered=false
        newPressed=false
        passwordPrompt=""
        masterEnabled=true; userEnabled=false; serviceEnabled=false; noteEnabled=false; webEnabled=false; pinEnabled=false
        selectedServer="none"
        newEnabled=true; addEnabled=false
        deleteEnabled=false; sureEnabled=false
    }
    
    // trim both ends of whitespace, lowercase everything, remove commas (maybe more?)
    func sanitise(_ s: String) -> String {
        var ns=s
        ns=ns.lowercased()
        ns=ns.trimmingCharacters(in: .whitespacesAndNewlines)
        ns=ns.replacingOccurrences(of: ",", with: "")
        return ns
    }
    
    // return true if three-in-a-row identical characters
    func triple(_ s: String) -> Bool {
        let c=Data(s.utf8)
        let len=c.count
        for i in 0..<len-2 {
            if c[i]==c[i+1] && c[i]==c[i+2] {
                return true
            }
        }
        return false
    }
    
    // return true if password meets policy
    // at least one special, one upper case, one lower case, one number
    // must start with a letter
    // must not contain triple repeat like aaa
    func policy(_ s: inout String) -> Bool {
        
        let f=s.first!
        if !f.isUppercase && !f.isLowercase {
            return false
        }
        
        s=s.replacingOccurrences(of: "/", with: "!")
        s=s.replacingOccurrences(of: "+", with: "$")
        s=s.replacingOccurrences(of: "I", with: "i")
        s=s.replacingOccurrences(of: "l", with: "L")
        
        if s.rangeOfCharacter(from: .uppercaseLetters) == nil || s.rangeOfCharacter(from: .lowercaseLetters) == nil || s.rangeOfCharacter(from: .decimalDigits) == nil || s.rangeOfCharacter(from: CharacterSet(charactersIn: "!$")) == nil
        {
            return false
        }
        
        if triple(s) {
            return false
        }
        
        return true
    }
    
    var body: some View {
        
        ScrollView {
            NavigationView {
                VStack(alignment : .leading,spacing: 8) {
                    HStack {
                        Text("Password Manager")
                            .bold()
                            .font(.title2)
                        Spacer()
                        Button(){
                            reset()
                        }
                        label: {
                            HStack (){
                                Text("Reset")
                            }
                            .padding()
                            .background(Color.red)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                        }
                    }
                    HStack {
                        TextField("",text:$label)
                            .frame(width: 72)
                            TextField(prompt,text:$master)
                            .textInputAutocapitalization(.never)
                            .disableAutocorrection(true)
                            .padding(5)
                            .focused($masterFocus)
                            //.disabled(newPressed)
                            .textFieldStyle(.roundedBorder)
                            .keyboardType(.asciiCapable)
                            .background(masterEnabled ? Color.red : Color.gray.opacity(0.2))
                            //.border(.secondary)
                            .onSubmit {
                                masterFocus=false
                                masterSecret=master
                                
                                digestMaster=Data(SHA3_512.hash(data: Data(masterSecret.utf8)))
                                //print("1. Master Digest",digestMaster[0],digestMaster[1],digestMaster[2])
                                for _ in 0..<20000 {
                                    digestMaster=Data(SHA3_512.hash(data: digestMaster))
                                }
                                //print("2. Master Digest",digestMaster[0],digestMaster[1],digestMaster[2])
                                
                                masterEntered=true
                                master=""
                                if !notfirstuse {
                                    saveRandFile()
                                }
                                username=""
                                if !notfirstuse {
                                    exit(0)
                                }
                                newEnabled=false
                                masterEnabled=false
                            }
                    }
                    HStack {
                        Text("Service")
                        
                        Picker("", selection: $selectedServer) {
                            ForEach(serverlist, id: \.self) {
                                //server in
                                Text($0)
                            }
                            
                        }
                        .onChange(of: selectedServer)
                        {
                            for index in 0..<list.count {
                                if list[index].service==selectedServer {
                                    username = list[index].username
                                    website = list[index].domain
                                    note = list[index].note
                                    length = String(list[index].policy_len)
                                    
                                    if selectedServer != "none" {
                                        deleteEnabled=true
                                        pinEnabled=true
                                        pinFocus=true
                                        chosenServer=selectedServer
                                        var concat=Data(chosenServer.utf8)
                                        //print("concat.count= ",concat.count)
                                        //print("concat= ",concat[0],concat[1],concat[2])
                                        
                                        concat.append(digestMaster)
                                        digestServer=Data(SHA3_512.hash(data: concat))
                                        
                                        //print("digestMaster= ",digestMaster[0],digestMaster[1],digestMaster[2])
                                        //print("Server Digest",digestServer[0],digestServer[1],digestServer[2])
                                        
                                    }
                                    break
                                }
                            }
                        }
                        //.pickerStyle(.wheel)
                        .disabled(!masterEntered)
                    }
                    HStack {
                        Text("Username")
                        TextField("Username",text:$username)
                            .textInputAutocapitalization(.never)
                            .disableAutocorrection(true)
                            .padding(5)
                            .textFieldStyle(.roundedBorder)
                            .keyboardType(.emailAddress)
                            .background(userEnabled ? Color.yellow : Color.gray.opacity(0.2))
                            //.border(.secondary)
                            .disabled(notfirstuse && !newPressed)
                    }
                    HStack {
                        Text("Website")
                        TextField("Website (Optional)",text:$website)
                            .textInputAutocapitalization(.never)
                            .disableAutocorrection(true)
                            .padding(5)
                            .textFieldStyle(.roundedBorder)
                            .keyboardType(.URL)
                            .background(webEnabled ? Color.yellow : Color.gray.opacity(0.2))
                            //.border(.secondary)
                            .disabled(!newPressed)
                    }
                    HStack {
                        Text("Note")
                        TextField("Note (Optional)",text:$note)
                            .padding(5)
                            .textFieldStyle(.roundedBorder)
                            .background(noteEnabled ? Color.yellow : Color.gray.opacity(0.2))
                            //.border(.secondary)
                            .disabled(!newPressed)
                    }
                    HStack {
                        Text("Enter PIN")
                        SecureField("PIN",text:$pin)
                            .padding(5)
                            .keyboardType(.numberPad) // enter after 4 numbers!
                            .frame(width: 80)
                            .focused($pinFocus)
                            .onChange(of: pin, {
                                pin = String(pin.prefix(pinLength))
                                if pin.count==pinLength {
                                    pinSecret=pin
                                    let pinInt=Int(pin)
                                    let top = UInt8(pinInt!/100)
                                    let bot = UInt8(pinInt!%100)
                                    let pinbytes=Data([bot,top])
                                    var concat=pinbytes
                                    concat.append(digestServer)
                                    digestPin=Data(SHA3_512.hash(data: concat))
                                    
                                    //print("PIN Digest",digestPin[0],digestPin[1],digestPin[2])
                                    
                                    concat=Data(randomSecret.utf8)
                                    concat.append(digestPin)
                                    
                                    var pw: String = ""
                                    repeat {
                                        concat = Data(SHA3_512.hash(data:concat))
                                        let final=concat.base64EncodedString()
                                        pw=String(final.prefix(Int(length)!))
                                    } while !policy(&pw)
                                    //print(pw)
                                    if show {
                                        password=pw
                                    } else {
                                        passwordPrompt = "in the clipboard"
                                    }
                                    UIPasteboard.general.string = pw
                                    pin=""
                                    username=""
                                    website=""
                                    note=""
                                    length=""
                                    newEnabled=false
                                    deleteEnabled=false
                                    selectedServer="none"
                                    pinEnabled=false
                                    pinFocus=false
                                    masterFocus=false
                                    masterEnabled=false
                                }
                            })
                            .textFieldStyle(.roundedBorder)
                            .background(pinEnabled ? Color.red : Color.gray.opacity(0.2))
                            
                        Button(){
                            // clear clipboard
                            password=""; passwordPrompt=""
                            UIPasteboard.general.string = "xxxxxxxxxxxxxxxx"
                        }
                        label: {
                            Spacer()
                            HStack {
                                Text("Clear")
                            }
                            .padding()
                            .background(Color.blue)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                        }
                    }
                    HStack{
                        Text("Password")
                        TextField(passwordPrompt,text:$password)
                            .font(Font.system(size: 14))
                            .padding(5)
                            //.border(.secondary)
                            //.disabled(true)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 150)
                        //TextField("",text:$show)
                        //    .border(.secondary)
                         //   .frame(width: 20)
                        Spacer()
                        //Text("Show")
                        Toggle("",isOn: $show)
                            
                    }
                    Divider()
                    HStack {
                        Button(){
                            //print("USER= ",user)
                            length="12"  // default
                            newPressed=true
                            serviceEnabled=true
                            webEnabled=true
                            userEnabled=true
                            noteEnabled=true
                            masterEnabled=false
                            username=user
                            serviceFocus=true
                            newEnabled=false
                            addEnabled=true
                        }
                        
                        label:  {
                            HStack {
                                Text("New")
                            }
                            .padding()
                            .background(newEnabled ? Color.blue: Color.gray)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                        }
                        .disabled(!notfirstuse || !newEnabled)
                        Spacer()
                        Text("Length")
                        TextField("",text:$length)
                            .border(.secondary)
                            .keyboardType(.decimalPad)
                            .frame(width:30)
                            .disabled(!notfirstuse)
                            .onChange(of: length, {
                                length = String(length.prefix(2))
                            })
                        Button(){
                            
                            newPressed=false
                            username=sanitise(username)
                            service=sanitise(service)
                            website=sanitise(website)
                            note=sanitise(note)
                            let newserver = Server(username: username,service: service,domain: website,note: note,policy_len:Int(length) ?? 12)
                            list.append(newserver)
                            serverlist.append(service)
                            // sort by servers
                            list[1..<list.count].sort{$0.service < $1.service}
                            serverlist[1..<serverlist.count].sort{$0 < $1}
                            
                            // create new sites.txt data
                            saveSites()
                            reset()
                            //newservice=service
                        }
                        label: {
                            HStack {
                                Text("Add")
                            }
                            .padding()
                            .background(addEnabled ? Color.blue : Color.gray)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                            
                        }
                        .disabled(!addEnabled)
                        
                    }
                    HStack{
                        Text("Service")
                        TextField("Service to be added or deleted",text:$service)
                            .focused($serviceFocus)
                            .textInputAutocapitalization(.never)
                            .disableAutocorrection(true)
                            .frame(width:200)
                            //.disabled(masterEntered)
                            
                            .padding(5)
                            .textFieldStyle(.roundedBorder)
                            .background(serviceEnabled ? Color.yellow : Color.gray.opacity(0.2))
                    }
                    HStack {
                        Button(){
                            serviceEnabled=true
                            service=selectedServer
                            deleteEnabled=false
                            sureEnabled=true
                        }
                        //.disabled(!notfirstuse || !newEnabled)
                        
                        label: {
                            HStack {
                                Text("Delete")
                            }
                            .padding()
                            .background(deleteEnabled ? Color.blue : Color.gray)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                        }
                        .disabled(!notfirstuse || !deleteEnabled)
                        
                        Spacer()
                        Button(){
                            var newlist:[Server] = []
                            var newserverlist:[String] = []
                            for line in list {
                                if line.service==selectedServer { continue}
                                newlist.append(line)
                            }
                            for line in serverlist {
                                if line==selectedServer { continue}
                                newserverlist.append(line)
                            }
                            list=newlist
                            serverlist=newserverlist
                            
                            saveSites()
                            
                            reset()
                            
                        }
                        label: {
                            HStack {
                                Text("Sure?")
                            }
                            .padding()
                            .background(sureEnabled ? Color.blue : Color.gray)
                            .foregroundColor(.white)
                            .cornerRadius(10)
                        }
                        .disabled(!sureEnabled)
                    }
                }
                .padding()
                //.navigationTitle("Password Manager")
            }
        }
        .padding()
        //.pickerStyle(.inline)
        
        .labelsHidden()
        .padding()
        
        .onAppear {
            serviceFocus=false
            masterFocus=true
        }
    }
}

#Preview {
    ContentView()
}
