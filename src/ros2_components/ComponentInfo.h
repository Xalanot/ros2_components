/*
 * Copyright 2018 <Lennart Nachtigall> <firesurfer127@gmail.com>
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
#pragma once


#include "ros2_components_msg/msg/list_components_response.hpp"

namespace ros2_components
{
/**
 * @brief ComponentInfo - Information needed to identify component.
 * Represents information about a certain component (e.g a motor)
 * These information allow to  definitely identify any component/entity in the system
 */
struct ComponentInfo
{

public:
    ComponentInfo();
    //Copy constructor
    ComponentInfo(const ComponentInfo &info);
    int64_t id;
    std::string type;
    std::string name;
    int64_t parentId;
    std::string parentType;
    std::vector<int64_t> childIds;
    std::vector<std::string> childTypes;
    uint32_t machineip;
    std::string nodename;
    bool subscriber;

    ros2_components_msg::msg::ListComponentsResponse::SharedPtr toRosMessage()
    {
        ros2_components_msg::msg::ListComponentsResponse::SharedPtr msg = std::make_shared<ros2_components_msg::msg::ListComponentsResponse>();
        //TODO check if all fields are correctly mapped
        msg->id = id;
        msg->type = type;
        msg->componentname = name;
        msg->parent = parentId;
        msg->parenttype = parentType;
        msg->childids = childIds;
        msg->childtypes = childTypes;
        msg->machineip = machineip;
        msg->nodename = nodename;
        msg->subscriber = subscriber;
        return msg;
    }
    std::string toString()
    {
        std::string ret =  "Name: " + name + " Type: " + type + " ParentId: " + std::to_string(parentId)  + " ParentType: " + parentType + " [";
        for(unsigned int i= 0; i < childIds.size(); i++)
        {
            ret +=   std::to_string(childIds[i]) + " , " + childTypes[i];
        }
        ret += "]";
        ret += " Node Name: " + nodename;
        ret += " Machineip: " + std::to_string(machineip);

        return ret;

    }
};

}


