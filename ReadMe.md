# 目录组织

- **Solver/**
  玻璃切割问题的 Visual C++ 项目.
  核心算法.

- **Simulator/**
  提供算法交互接口的 Visual C++ 项目.
  实现批量测试与结果展示等功能.

- **Deploy/**
  开发阶段程序部署目录, 包含程序运行所需要的所有可执行文件和数据.
  - **Instance/**
    输入数据.
    从比赛官网或者 https://coding.net/u/SuZhouxing/p/GuillotineCut.Doc/git 下载算例文件并拷贝至该目录.
  - **Solution/**
    求解结果.
  - **Visualization/**
    结果可视化.

- **Doc/**
  项目文档.
  - **CuttingModel.md**
    玻璃切割问题模型.
  - **PackingModel.md**
    平面装配问题模型.

- **Lib/**
  第三方库.
  - **gurobi**
    高效的商业混合整数规划求解器.
    访问 http://www.gurobi.com 下载最新版本并将安装目录内的 `include/` 和 `lib/` 目录拷贝至该目录.
    - **include/**
      头文件. 应被设置为 "附加包含文件".
    - **lib/**
      静态链接库. 应被设置为 "附加库目录".
      该文件夹下的库文件在 Visual Studio 中将通过 Auto Linking 技术自动链接.



# 编译链接

## Windows

打开根目录下的解决方案文件 `GuillotineCut.sln` 生成即可.


## Linux

- Debug
  `g++ -m64 -g -o challengeSG Solver/*.cpp -ILib/gurobi/include -LLib/gurobi/lib/ -lgurobi_c++ -lgurobi80 -lm`
- Release
	`g++ -m64 -O2 -o challengeSG Solver/*.cpp -ILib/gurobi/include -LLib/gurobi/lib/ -lgurobi_c++ -lgurobi80 -lm`
