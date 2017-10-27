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

#pragma once
/*Qt*/
#include <QObject>

/*stdlib*/
#include <functional>
#include <cstdlib>
#include <iostream>
#include <ctime>
#include <list>
#include <memory>
#include <chrono>
#include <mutex>
#include <condition_variable>

/*ROS2*/
#include "rclcpp/rclcpp.hpp"

/*Messages*/
#include "ros2_components_msg/msg/component_changed.hpp"
#include "ros2_components_msg/msg/list_components_request.hpp"
#include "ros2_components_msg/msg/list_components_response.hpp"

/*ros2_components*/
#include "ros2_simple_logger/Logger.h"

#include "AdvertisementType.h"
#include "EntityFactory.h"
#include "ComponentInfo.h"
#include "ComponentInfoFactory.h"

namespace ros2_components
{
/**
 * @brief The ComponentManager class - Collects component/entity advertisements in the systems and represents them ordered to the user.
 */
class ComponentManager:public QObject
{
    Q_OBJECT
public:
    //Typedef for easy to use SharedPtr
    typedef std::shared_ptr<ComponentManager> SharedPtr;
    /**
     * @brief ComponentManager
     * @param _localNode
     * Use this constructor if your having a node that doesnt need a base entity (for example an algorithm node)
     */
    ComponentManager(rclcpp::node::Node::SharedPtr _localNode);
    /**
     * @brief ComponentManager
     * @param _localNode
     * @param _baseEntity
     * Use this constructor if your having a node that needs a base entity (for example a hardware sensor/actor node)
     */
    ComponentManager(rclcpp::node::Node::SharedPtr _localNode, EntityBase::SharedPtr _baseEntity);
    /**
     * @brief ~ComponentManager - destructor. Waits for responder thread
     */
    virtual ~ComponentManager();
    /**
     * @brief ListComponents
     * @return vector of all currently listed components
     * TODO think of a better way to access all components
     */
    std::vector<ComponentInfo> ListComponents();
    /**
     * @brief ListNodes
     * @return List of all found nodes
     */
    std::vector<std::string> ListNodes(); //TODO move?
    /**
     * @brief ListComponentsBy
     * @param filter
     * @return List of ComponentInfo objects that are matched by the given ComponentListFilter
     */
    std::vector<ComponentInfo> ListComponentsBy(std::function<bool(const ComponentInfo&)> filter);
    /**
     * @brief GetInfoWithFilter gets the first Component which matches the filter
     * @param filter the filter
     * @param success will be set to true if a Component was found before timeout and false if not
     * @param timeout in milliseconds. Waits indefinitely if negative
     * @throws std::runtime_error if a rclcpp::executor is already running, and this method waits for the Component. Will not happen with a timeout of 0ms
     */
    ComponentInfo GetInfoWithFilter(std::function<bool(const ComponentInfo&)> filter, bool* success = nullptr, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());
    /**
     * @brief GetInfoToId
     * @param id
     * @param success will be set to true if a Component was found before timeout and false if not
     * @param timeout in milliseconds. Waits indefinitely if negative
     * @throws std::runtime_error if a rclcpp::executor is already running, and this method waits for the Component. Will not happen with a timeout of 0ms.
     */
    ComponentInfo GetInfoToId(int64_t id, bool* success = nullptr, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());
    /**
     * @brief GetInfoWithFilterAsync gets the first Component which matches the filter and calls the callback on it
     * @param callback the callback to call when the component is found
     * @param filter the filter
     * @param timeout will call @callback with an default-initialized ComponentInfo when it timeouts. Will not time out if @param timeout <= 0ms
     */
    void GetInfoWithFilterAsync(std::function<void(ComponentInfo)> callback, std::function<bool(const ComponentInfo&)> filter, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());
    /**
     * @brief GetInfoWithFilterAsync gets the first Component with given id and calls the callback on it
     * @param callback the callback to call when the component is found
     * @param id the id
     * @param timeout will call @callback with an default-initialized ComponentInfo when it timeouts. Will not time out if @param timeout <= 0ms
     */
    void GetInfoToIdAsync(std::function<void(ComponentInfo)> callback, int64_t id, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());
    /**
     * @brief UpdateComponentsList
     * Publishes a message to the ListComponentsRequest topic.
     */
    void UpdateComponentsList();

    /**
     *  Rebuild first Component which matches the given filter, pass true for rebuild Hierarchy to rebuild an entity tree (for example give the robot and and pass true in order to rebuild the whole component tree)
     *  @param rebuildHierarchy, wait for and rebuild all childs as well, defaults to false
     *  @param timeout in milliseconds, throws exception if no component is found before timeout. Waits indefinitely if negative; defaults to 0ms
     */
    template<typename T>
    std::shared_ptr<T> RebuildComponent(std::function<bool(const ComponentInfo&)> filter, bool rebuildHierarchy = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        auto startTime = std::chrono::system_clock::now();
        bool waitIndefinitely = timeout < std::chrono::milliseconds::zero();
        bool found;
        ComponentInfo relevantInfo = GetInfoWithFilter(filter, &found, timeout);

        if (found)
        {
            std::shared_ptr<T> ent;
            if (waitIndefinitely)
            {
                ent = RebuildComponent<T>(relevantInfo, rebuildHierarchy, timeout);
            }
            else
            {
                auto remaining_timeout = startTime + timeout - std::chrono::system_clock::now();
                if (remaining_timeout < std::chrono::milliseconds::zero())
                {
                    remaining_timeout = std::chrono::milliseconds::zero();
                }
                ent = RebuildComponent<T>(relevantInfo, rebuildHierarchy, std::chrono::duration_cast<std::chrono::milliseconds>(remaining_timeout));
            }
            return ent;
        }
        else
        {
            throw  std::runtime_error("Could not find Component in time"); //FIXME more descriptive exception
        }
    }
    /**
     *  Rebuild Component from the given id, pass true for rebuild Hierarchy to rebuild an entity tree (for example give the robot and and pass true in order to rebuild the whole component tree)
     *  @param rebuildHierarchy, wait for and rebuild all childs as well, defaults to false
     *  @param timeout in milliseconds, throws exception if no component is found before timeout. Waits indefinitely if negative; defaults to 0ms
     */
    template<typename T>
    std::shared_ptr<T> RebuildComponent(int64_t id, bool rebuildHierarchy = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        auto filter = [id] (const ComponentInfo& info) -> bool
        {
            return (info.id == id);
        };
        return RebuildComponent<T>(filter, rebuildHierarchy, timeout);
    }

    /**
     *  Rebuild first Component which matches the given filter and call the callback with it, will return immediately
     *  @param filter the filter
     *  @param rebuildHierarchy, wait for and rebuild all childs as well, defaults to false
     *  @param timeout in milliseconds, will pass empty shared_ptr to the callback if it times out. Waits indefinitely if <= 0ms; defaults to 0ms
     */
    template<typename T>
    void RebuildComponentAsync(std::function<void(std::shared_ptr<T>)> callback, std::function<bool(const ComponentInfo&)> filter, bool rebuildHierarchy = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        if (rebuildHierarchy)
        {
            //TODO test this extensively
            auto timeoutTimePoint = std::chrono::system_clock::now() + timeout;
            std::shared_ptr<uint64_t> subComponentCount = std::make_shared<uint64_t>(0);
            std::shared_ptr<int64_t> parentId = std::make_shared<int64_t>(0);
            std::function<void(ComponentInfo)> parentCallback = [callback, parentId, timeoutTimePoint, subComponentCount, &parentCallback, this] (ComponentInfo childInfo)
            {
                if (childInfo.name.empty())
                {
                    //Timeout
                    callback(std::shared_ptr<T>());
                    return;
                }
                if (*parentId == 0)
                {
                    *parentId = childInfo.id;
                }
                *subComponentCount += childInfo.childIds.size();
                for (auto childId : childInfo.childIds)
                {
                    this->GetInfoToIdAsync(parentCallback, childId, std::chrono::duration_cast<std::chrono::milliseconds>(timeoutTimePoint - std::chrono::system_clock::now()));
                }
                if (*subComponentCount == 0)
                {
                    //All callbacks have been called, so all childs are available
                    std::shared_ptr<T> entity = RebuildComponent<T>(*parentId, true);
                    if(!entity)
                        throw std::runtime_error("Could not cast entity to given type");
                    callback(entity);
                }
                else
                {
                    (*subComponentCount)--;
                }
            };
            GetInfoWithFilterAsync(parentCallback, filter, timeout);
        }
        else
        {
            auto fullCallback = [callback, this] (ComponentInfo info)
            {
                if (info.name.empty())
                {
                    callback(std::shared_ptr<T>());
                    return;
                }
                std::shared_ptr<T> entity = dynamic_pointer_cast<T>(RebuildComponent(info, true, false));
                if(!entity)
                    throw std::runtime_error("Could not cast entity to given type");
                callback(entity);
            };
            GetInfoWithFilterAsync(fullCallback, filter, timeout);
        }
    }
    /**
     *  Rebuild Component from the given id and call the callback with it, will return immediately
     *  @param id the id of the component
     *  @param rebuildHierarchy, wait for and rebuild all childs as well, defaults to false
     *  @param timeout in milliseconds, will pass empty shared_ptr to the callback if it times out. Waits indefinitely if <= 0ms; defaults to 0ms
     */
    template<typename T>
    void RebuildComponentAsync(std::function<void(std::shared_ptr<T>)> callback, int64_t id, bool rebuildHierarchy = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        auto filter = [id] (const ComponentInfo& info) -> bool
        {
            return (info.id == id);
        };
        RebuildComponentAsync(callback, filter, rebuildHierarchy, timeout);
    }

    /**
     * Rebuild a component from a ComponentInfo object using the EntityFactory
     */
    template<typename T>
    std::shared_ptr<T> RebuildComponent(ComponentInfo & info,bool rebuildHierarchy = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        std::shared_ptr<T> entity = dynamic_pointer_cast<T>(RebuildComponent(info, rebuildHierarchy, false, timeout));
        if(!entity)
            throw std::runtime_error("Could not cast entity to given type");
        return entity;
    }
    /**
     * RebuildComponent helper method
     */
    std::shared_ptr<EntityBase> RebuildComponent(const ComponentInfo & info, bool rebuildHierarchy = false, bool forcePubOrSubChange = false, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero());

private:
    /*Ros2 stuff*/
    /**
     * @brief RosNode
     */
    rclcpp::node::Node::SharedPtr RosNode;
    /**
     * @brief ComponentChangedSubscription
     * This topic keeps you informed about changes to components (entities) in the system - but not about new components
     */
    std::shared_ptr<rclcpp::subscription::Subscription<ros2_components_msg::msg::ComponentChanged>> ComponentChangedSubscription;
    /**
     * @brief ListComponentsRequestSubscription
     * On this topic a component manager can publish a request to all other component manager instances in the system in order to have them publish all their managed components to the
     * ListComponentResponse topic
     */
    rclcpp::subscription::Subscription<ros2_components_msg::msg::ListComponentsRequest>::SharedPtr ListComponentsRequestSubscription;
    /**
     * @brief ListComponentsResponseSubscription
     * On this topic the answer to the ListComponentsRequest is published
     */
    rclcpp::subscription::Subscription<ros2_components_msg::msg::ListComponentsResponse>::SharedPtr ListComponentsResponseSubscription;

    /**
     * @brief ListComponentsRequestPublisher
     * @see ListComponentsRequestSubscription
     */
    rclcpp::publisher::Publisher<ros2_components_msg::msg::ListComponentsRequest>::SharedPtr ListComponentsRequestPublisher;
    /**
     * @brief ListComponentsResponsePublisher
     * @see ListComponentsResponseSubscription
     */
    rclcpp::publisher::Publisher<ros2_components_msg::msg::ListComponentsResponse>::SharedPtr ListComponentsResponsePublisher;

    /**
     * @brief ListComponentsRequestCallback
     * @param msg
     * Gets called if a new message on the ListComponentsRequest topic is available
     */
    void ListComponentsRequestCallback(ros2_components_msg::msg::ListComponentsRequest::SharedPtr msg);
    /**
     * @brief ListComponentsResponseCallback
     * @param msg
     * Gets called if a new message on the ListComponentsResponse topic is available
     */
    void ListComponentsResponseCallback(ros2_components_msg::msg::ListComponentsResponse::SharedPtr msg);

    /**
     * @brief Components
     * Stored components
     */
    std::vector<ComponentInfo> Components;
    std::mutex componentsMutex; //TODO Test this extensively
    uint64_t componentsReader;
    std::condition_variable componentsCV;
    EntityBase::SharedPtr BaseEntity;
    rmw_qos_profile_t component_manager_profile;
    void GenerateResponse();
    rclcpp::timer::TimerBase::SharedPtr updateTimer;

    std::list<QMetaObject::Connection> callbacks;
    std::mutex callbacksMutex;

    //RAII helper to manage reader entrance on Components
    class ReaderGuard
    {
    public:
        ReaderGuard(ComponentManager*);
        ~ReaderGuard();

    private:
        ComponentManager* cm;
    };

signals:
    //Qt signals that can be used in order to stay informed about changes in the system
    void NewComponentFound(ComponentInfo info);
    void ComponentDeleted(ComponentInfo info);
    void ComponentChanged(ComponentInfo info);
private slots:
    void OnChildAdded(EntityBase::SharedPtr child, EntityBase::SharedPtr parent, bool remote);
    void OnChildRemoved(EntityBase::SharedPtr child, EntityBase::SharedPtr parent, bool remote);
    void OnEntityDeleted(ComponentInfo info);
};
}


