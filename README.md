
一个用C++和SDL3编写的初代GameBoy模拟器


你需要安装有xmake,然后使用`xmake`来编译

##### 使用方法
在项目根目录下的roms文件夹添加rom，后缀为gb（支持部分gbc文件）
然后运行`.\GBemu.exe "you_rom.gb" 2`,最后一个参数为倍率，不写或者写1为原速

##### 键盘映射
| GameBoy按键 | 键盘按键 |
| ----------- | --------|
| D-Pad ↑     | W    |
| D-Pad ↓     | S    |
| D-Pad ←     | A    |
| D-Pad →     | D    |
| A           | J    |
| B           | K    |
| Start       | I    |
| Select      | U    |

##### 联机
需要在同一个WIFI下
当输入`.\GBemu.exe "you_rom.gb" 2`作为主机（默认使用端口8765）
输入`.\GBemu.exe "you_rom.gb" 2 "xxx.xxx.x.xx"`为客户端，最后的参数为主机的`IPv4 地址`，可让主机通过`ipconfig`来查看
- 注意：需双方速率一样，且主机先运行