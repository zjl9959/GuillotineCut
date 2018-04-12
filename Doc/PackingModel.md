<p style="text-align: center; font-size: 32px; font-weight:bold;">
  2D Rectangle Packing with Rotation
</p>



[TOC]



# 2D Rectangle Packing with Rotation

## Known

### Set

| Set | Description                   | Size        | Element         | Remark                                                         |
| ---- | ---------------------- | ----------- | ------------ | ------------------------------------------------------------ |
| $I$  | (**i**tem) item set | $[1, 200]$  | $i, j$ |               |

### Constant

| Constant | Description                    | Type | Range       | Remark |
| -------- | ------------------------------ | ---- | ----------- | ------ |
| $W$      | the width of the raw material  | real | $(0, 1000]$ |        |
| $H$      | the height of the raw material | real | $(0, 1000]$ |        |
| $w_{i}$  | the width of the item $i$      | real | $(0, W]$    |        |
| $h_{i}$  | the height of the item $i$     | real | $(0, H]$    |        |


## Decision

| Variable     | Description                                                | Type | Domain     | Remark                                                     |
| -------------- | ------------------------------------------------------------ | ---- | ------------- | ------------------------------------------------------------ |
| $x_{i}$       | horizontal position of the item $i$'s left bottom                           | real | $[0, W]$ | in Cartesian coordinate system |
| $y_{i}$ | vertical position of the item $i$'s left bottom | real | $[0, H]$ | in Cartesian coordinate system |
| $d_{i}$       | item $i$ is rotated 90 degree              | bool  | $\{0, 1\}$ |                                              |
| $r_{ij}$       | item $i$ is on the right side of item $j$ | bool  | $\{0, 1\}$ |                                                              |
| $u_{ij}$  | item $i$ is on the up side of item $j$ | bool | $\{0, 1\}$  |                                                              |

### Convention and Function

- define function $\textrm{w}(i) = w_{i} \cdot (1 - d_{i}) + h_{i} \cdot d_{i}$ to indicate the actual width of item $i$ regarding its rotation.
- define function $\textrm{h}(i) = h_{i} \cdot (1 - d_{i}) + w_{i} \cdot d_{i}$ to indicate the actual height of item $i$ regarding its rotation.


## Objective

there is no objective since it is a decision problem instead of an optimization problem.


## Constraint

- hard constraints
  - prefixed with **H** in the tag
  - constraints in the original problem that must be satisfied
- soft constraints
  - prefixed with **S** in the tag
  - objectives in the original problem
  - cut-off scores to the low-priority objectives in case the high-priority objectives are over dominant
- auxiliary constraints
  - prefixed with **A** in the tag
  - additional constraints for converting non-linear constraints into linear ones
- optional constraints
  - prefixed with **.O** in the tag
  - constraints that are very likely to be ignored when changing requirement or implementation
- lazy constraints
  - suffixed with **.L** in the tag
  - constraints which are recommended to be added lazily

- **HIHB (inside horizontal bound)** item $i$ is placed within the raw material horizontally.
$$
x_{i} + \textrm{w}(i) \le W, \quad \forall i \in I
$$
- **HIVB (inside vertical bound)** item $i$ is placed within the raw material vertically.
$$
y_{i} + \textrm{h}(i) \le H, \quad \forall i \in I
$$

- **HHED (horizontal exclusive definition)** 
$$
x_{i} + W \cdot (1 - r_{ij}) \ge x_{j} + \textrm{w}(j), \quad \forall i, j \in I
$$
- **HVED (vertical exclusive definition)** 
$$
y_{i} + H \cdot (1 - u_{ij})  \ge y_{j} + \textrm{h}(j), \quad \forall i, j \in I
$$

- **HHRP (horizontal relative position)** 
$$
r_{ij} + r_{ji} \le 1, \quad \forall i, j \in I
$$
- **HVRP (vertical relative position)** 
$$
u_{ij} + u_{ji} \le 1, \quad \forall i, j \in I
$$

- **HHVE (horizontal and vertical exclusive)** 
$$
r_{ij} + r_{ji} + u_{ij} + u_{ji} \ge 1, \quad \forall i, j \in I
$$
