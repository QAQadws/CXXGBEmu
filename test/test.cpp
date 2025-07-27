#include <gtest/gtest.h>
#include <string>

// 最简单的测试，验证 gtest 是否正常工作
TEST(BasicTest, SimpleAssertions) {
    // 基本相等测试
    EXPECT_EQ(1, 1);
    EXPECT_EQ(2 + 2, 4);
    
    // 布尔测试
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    
    // 不等测试
    EXPECT_NE(1, 2);
}

// 测试数学运算
TEST(MathTest, BasicMath) {
    // 加法测试
    EXPECT_EQ(5 + 3, 8);
    EXPECT_EQ(-1 + 1, 0);
    
    // 乘法测试
    EXPECT_EQ(3 * 4, 12);
    EXPECT_EQ(0 * 100, 0);
    
    // 除法测试
    EXPECT_EQ(10 / 2, 5);
    EXPECT_DOUBLE_EQ(7.0 / 2.0, 3.5);
}

// 测试字符串操作
TEST(StringTest, BasicStringOperations) {
    std::string hello = "Hello";
    std::string world = "World";
    std::string result = hello + " " + world;
    
    EXPECT_EQ(result, "Hello World");
    EXPECT_NE(hello, world);
    EXPECT_EQ(hello.length(), 5);
    EXPECT_TRUE(hello.find("ell") != std::string::npos);
}

// GameBoy 相关的基础测试（为将来的开发做准备）
TEST(GameBoyTest, Constants) {
    // GameBoy 的一些基本常量
    const int SCREEN_WIDTH = 160;
    const int SCREEN_HEIGHT = 144;
    const int MEMORY_SIZE = 0x10000;  // 64KB
    const int CPU_FREQUENCY = 4194304; // 4.194304 MHz
    
    EXPECT_EQ(SCREEN_WIDTH, 160);
    EXPECT_EQ(SCREEN_HEIGHT, 144);
    EXPECT_EQ(MEMORY_SIZE, 65536);
    EXPECT_GT(CPU_FREQUENCY, 4000000);
}

// 测试数组和容器
TEST(ContainerTest, ArrayAndVector) {
    // 数组测试
    int numbers[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(numbers[0], 1);
    EXPECT_EQ(numbers[4], 5);
    
    // Vector 测试
    std::vector<int> vec = {10, 20, 30};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec.back(), 30);
}

// 错误处理测试
TEST(ErrorTest, ExceptionHandling) {
    // 测试除零异常（这里只是示例，实际不会抛异常）
    int a = 10;
    int b = 2;
    EXPECT_NO_THROW(a / b);
    
    // 测试范围检查
    std::vector<int> vec = {1, 2, 3};
    EXPECT_LT(0, vec.size());  // 确保不为空
    EXPECT_GE(vec.size(), 1);  // 至少有一个元素
}

// 手动添加 main 函数，因为自动配置的 gtest_main 有问题
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}