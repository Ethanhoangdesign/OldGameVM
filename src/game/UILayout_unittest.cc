#include "gtest/gtest.h"

#include "UILayout.h"

TEST(UILayoutTest, Centers934x480MapWithoutScaling)
{
	UILayout layout(934, 480);

	EXPECT_EQ(layout.get_MAP_VIEW_START_X(), 500);
	EXPECT_EQ(layout.get_MAP_VIEW_START_Y(), 31);
	EXPECT_EQ(layout.get_MAP_VIEW_WIDTH(), 336);
	EXPECT_EQ(layout.get_MAP_VIEW_HEIGHT(), 298);
	EXPECT_EQ(layout.get_MAP_GRID_X(), 21);
	EXPECT_EQ(layout.get_MAP_GRID_Y(), 18);
}
