#include <gtest/gtest.h>
#include "messages/coordinator.hpp"
#include "messages/publisher.hpp"
#include "messages/subscriber.hpp"
#include "messages/message.hpp"

using namespace ember::messaging;

TEST(PubSubTest, DecoupledDelivery) {
    Coordinator coordinator;
    auto sub = std::make_shared<Subscriber>("telemetry/gps");
    coordinator.subscribe(sub);

    Publisher pub(&coordinator);
    Message msg{"telemetry/gps", std::chrono::steady_clock::now(), "LAT:37.7749,LON:-122.4194"};
    pub.push(msg);

    Message received;
    EXPECT_TRUE(sub->pop(received));
    EXPECT_EQ(received.topic, "telemetry/gps");
    EXPECT_EQ(received.payload, "LAT:37.7749,LON:-122.4194");
}

TEST(PubSubTest, TopicFiltering) {
    Coordinator coordinator;
    auto sub_a = std::make_shared<Subscriber>("topic/alpha");
    auto sub_b = std::make_shared<Subscriber>("topic/beta");

    coordinator.subscribe(sub_a);
    coordinator.subscribe(sub_b);

    Publisher pub(&coordinator);
    Message msg_a{"topic/alpha", std::chrono::steady_clock::now(), "AlphaData"};
    pub.push(msg_a);

    Message rec_a;
    EXPECT_TRUE(sub_a->pop(rec_a));
    EXPECT_EQ(rec_a.payload, "AlphaData");

    Message rec_b;
    EXPECT_FALSE(sub_b->pop(rec_b)); // sub_b should receive nothing
}

TEST(PubSubTest, MultipleSubscribersFanOut) {
    Coordinator coordinator;
    auto sub1 = std::make_shared<Subscriber>("system/status");
    auto sub2 = std::make_shared<Subscriber>("system/status");
    auto sub3 = std::make_shared<Subscriber>("system/status");

    coordinator.subscribe(sub1);
    coordinator.subscribe(sub2);
    coordinator.subscribe(sub3);

    Publisher pub(&coordinator);
    Message broadcast{"system/status", std::chrono::steady_clock::now(), "OK"};
    pub.push(broadcast);

    Message m1, m2, m3;
    EXPECT_TRUE(sub1->pop(m1));
    EXPECT_TRUE(sub2->pop(m2));
    EXPECT_TRUE(sub3->pop(m3));

    EXPECT_EQ(m1.payload, "OK");
    EXPECT_EQ(m2.payload, "OK");
    EXPECT_EQ(m3.payload, "OK");
}
