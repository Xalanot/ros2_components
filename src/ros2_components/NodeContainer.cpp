#include "NodeContainer.h"

namespace ros2_components {


NodeContainer::NodeContainer(rclcpp::node::Node::SharedPtr _ros_node, int64_t _node_id, std::string _name):
    ros_node(_ros_node), node_id(_node_id), node_name(_name)
{

}

NodeContainer::~NodeContainer()
{
    abort = true;
    if(spinThread)
        spinThread->join();
}

void NodeContainer::Spin(std::chrono::nanoseconds timeout)
{
    if(isSpinning)
        throw std::runtime_error("Node already spinning");
    isSpinning = true;
    if(timeout == std::chrono::nanoseconds(-1))
        rclcpp::spin_some(ros_node);
    else
    {
        executor.add_node(ros_node);
        executor.spin_once(timeout);
        executor.remove_node(ros_node);

    }
    isSpinning = false;
}

void NodeContainer::SpinAsync()
{
    if(isSpinning)
        throw std::runtime_error("Node already spinning");
    auto async_spin = [&]()
    {
        isSpinning = true;
        rclcpp::WallRate loopRate(this->loop_rate);
        while(!abort && rclcpp::ok())
        {
            rclcpp::spin_some(ros_node);
            loopRate.sleep();

        }
        isSpinning = false;
        isSpinningAsync = false;
    };

    spinThread = std::make_shared<std::thread>(async_spin);
    this->isSpinningAsync = true;
}

rclcpp::node::Node::SharedPtr NodeContainer::GetRosNode()
{
    return ros_node;
}

bool NodeContainer::GetIsSpinning() const
{
    return isSpinning;
}

bool NodeContainer::GetIsSpinningAsync() const
{
    return isSpinningAsync;
}

int NodeContainer::GetLoopRate() const
{
    return loop_rate;
}

void NodeContainer::SetLoopRate(int value)
{
    loop_rate = value;
}

std::string NodeContainer::GetNodeName() const
{
    return node_name;
}

int64_t NodeContainer::GetNodeId() const
{
    return node_id;
}
}
