#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Return;

// ---- TEST BASE ----
TEST(PlaceholderTest, AlwaysTrue)
{
    EXPECT_EQ(1, 1);
}

// ---- ESEMPIO LOGICA ----
int add(int a, int b)
{
    return a + b;
}

TEST(MathTest, AdditionWorks)
{
    EXPECT_EQ(add(2, 3), 5);
}

// ---- MOCK ESEMPIO ----
class IFoodSource
{
public:
    virtual ~IFoodSource() = default;
    virtual int getFood() = 0;
};

class MockFoodSource : public IFoodSource
{
public:
    MOCK_METHOD(int, getFood, (), (override));
};

TEST(MockTest, UsesGoogleMock)
{
    MockFoodSource mock;

    EXPECT_CALL(mock, getFood())
        .Times(1)
        .WillOnce(Return(42));

    EXPECT_EQ(mock.getFood(), 42);
}