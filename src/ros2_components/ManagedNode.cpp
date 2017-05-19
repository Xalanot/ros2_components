/*
 * Copyright 2016 <Lennart Nachtigall> <firesurfer65@yahoo.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "ManagedNode.h"
namespace ros2_components {


ManagedNode::ManagedNode(std::string nodeName, int argc, char *argv[]):cliParser{argv,argc,nodeName}
{

    //Initialise ros2
    rclcpp::init(argc, argv);

    //Parse commandline arguments for --id and --logpath
    //TODO make this nicer ;)

    int64_t id = 100;
    std::string id_str= "";

    //TODO put addition help information int parser

    this->LogfilePath = "";
    this->ConfigfilePath = "settings.xml";

    cliParser.addArgument(std::make_shared<CLIArgument>("id","Specify id used for the node", &id_str));
    cliParser.addArgument(std::make_shared<CLIArgument>("logpath","Path to the logfile - also enables the logging to a file", &this->LogfilePath));
    cliParser.addArgument(std::make_shared<CLIArgument>("configpath","Path to a configfile", &this->ConfigfilePath));
    cliParser.parse();
    if(cliParser.getHelpFound())
        exit(0);
    if(id_str != "")
    {
        std::cout << id_str << std::endl;
        id = std::stoi(id_str);
    }
    for(int i = 0; i < argc;i++)
    {
        std::string arg = std::string(argv[i]);
        this->CommandLineArguments.push_back(arg);
        /*if(arg.find("--id=") != std::string::npos)
        {
            std::string id_str = arg.erase(0, arg.find_first_of('=')+1);

            id = std::stoi(id_str);
        }
        else if(arg.find("--logpath=") != std::string::npos)
        {
            std::string logfilePath = arg.erase(0, arg.find_first_of('=')+1);
            this->LogfilePath = logfilePath;
        }
        else if(arg.find("--configpath=") != std::string::npos)
        {
            //TODO add checks
            std::string ConfigfilePath = arg.erase(0, arg.find_first_of('=')+1);
        }*/

    }

    RosNode = rclcpp::node::Node::make_shared(nodeName+ std::to_string(id));
    NodeId = id;
    std::cout << "Started node: " << RosNode->get_name() << std::endl;

}
ManagedNode::~ManagedNode()
{
    Exit();
}

void ManagedNode::Spin()
{
    if(!isSetup)
        throw std::runtime_error("Node wasn't setup yet");
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(RosNode);
    rclcpp::WallRate loop_rate(80);
    while(rclcpp::ok() && !Abort)
    {
        executor.spin_some();
        loop_rate.sleep();

    }
    // std::cout << "Ending spin" << std::endl;
}

bool ManagedNode::Ok()
{
    return rclcpp::ok();
}

ComponentManager::SharedPtr ManagedNode::GetComponentManager()
{
    return this->CompManager;
}

rclcpp::node::Node::SharedPtr ManagedNode::GetRosNode()
{
    return this->RosNode;
}

int64_t ManagedNode::GetNodeId()
{
    return this->NodeId;
}

bool ManagedNode::NodeSetupSuccessfull()
{
    return this->isSetup;
}

void ManagedNode::AsyncWorker()
{
    //TODO make loop_rate configurable
    rclcpp::WallRate loop_rate(80);
    while(!Abort)
    {
        DoWork();
        loop_rate.sleep();
    }
}


void ManagedNode::DoWork()
{

}

void ManagedNode::Exit()
{
    //std::cout << "Calling exit" << std::endl;
    Abort = true;
    if(WorkThread)
        WorkThread->join();
    if(SpinThread)
        SpinThread->join();
    //std::cout << "Thread successfully ended" << std::endl;
    rclcpp::shutdown();
    //std::cout << "Exit successfull" << std::endl;
}

void ManagedNode::Start(bool multithreaded)
{
    SpinThread = std::make_shared<std::thread>(std::bind(&ManagedNode::Spin,this));
    if(multithreaded)
        WorkThread = std::make_shared<std::thread>(std::bind(&ManagedNode::AsyncWorker,this));
}
void ManagedNode::Setup()
{
    Setup(LogLevel::Info);

}
void ManagedNode::Setup(LogLevel logLevel)
{
    //If you want to implement a hardware node create base entity in derived class
    if( !this->RosNode)
        throw std::runtime_error("RosNode may not be null - cant proceed with setup");

    //Init logger
    INIT_LOGGER(RosNode);
    //Set loglevel to given loglevel
    LOGLEVEL(logLevel);
    //If the --logfile argument was successfully parsed, set logfilepath (this will enable logging to a file)
    if(this->LogfilePath != "")
        simpleLogger::getInstance()->setLogFilePath(this->LogfilePath);

    //Component manager for a hardware node: Has a baseentity where to hardware model orignates
    if(this->BaseEntity)
        this->CompManager = std::make_shared<ComponentManager>(this->RosNode,this->BaseEntity);
    else
        this->CompManager = std::make_shared<ComponentManager>(this->RosNode);
    //Component manager for a algorithm node: Doesnt need a baseentity
    this->isSetup = true; //Set setup to true
}

}


