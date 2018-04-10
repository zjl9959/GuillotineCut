<p style="text-align: center; font-size: 32px; font-weight:bold;">
  Guillotine Cut
</p>



[TOC]



# ROADEF Challenge 2018 Qualification

## Known

### Set

| Set | Description                   | Size        | Element         | Remark                                                         |
| ---- | ---------------------- | ----------- | ------------ | ------------------------------------------------------------ |
| $I$  | (**i**tem) item set | $[1, 200]$  | $i, j$ |               |
| $S$ | (**s**tack) ordered list of items | $[1, 200]$  | $s$ |               |
| $B$ | (**b**atch) stack set | $[1, 200]$  | $b$ |               |
| $D$ | (**d**efect) defect set | $[1, 200]$  | $d$ |               |
| $G$ | (**g**lass) raw material set | $[1, 200]$ | $g$ |               |
| $C$ | (**c**ut) cut set | $[1, 200]$ | $c$ | |
| $L^{1}$ | layer-1 virtual bin |  | $l, l'$ | |
| $L^{2}_{l}$ | layer-2 virtual bin |  | $m, m'$ | |
| $L^{3}_{lm}$ | layer-3 virtual bin |  | $n, n'$ | |

### Constant

| Constant | Description                    | Type | Range       | Remark |
| -------- | ------------------------------ | ---- | ----------- | ------ |
| $W$      | the width of the raw material  | real | $(0, 1000]$ |        |
| $H$      | the height of the raw material | real | $(0, 1000]$ |        |
| $w_{i}$  | the width of the item $i$      | real | $(0, W]$    |        |
| $h_{i}$  | the height of the item $i$     | real | $(0, H]$    |        |


## Decision

| Variable     | Description                                                | type | Domain     | Remark                                                     |
| -------------- | ------------------------------------------------------------ | ---- | ------------- | ------------------------------------------------------------ |
| $d_{i}$       | item $i$ is rotated 90 degree            | bool  | $\{0, 1\}$ |                                              |
| $\omega^{1}_{l}$ | width of the layer-1 virtual bin $l$ | real | $[0, W]$ | |
| $\eta^{2}_{lm}$ | height of the layer-2 virtual bin $m$ in L1 virtual bin $l$ | real | $[0, H]$ | |
| $\omega^{3}_{lmn}$ | width of the layer-3 virtual bin $n$ in L2 virtual bin $(l, m)$ | real | $[0, W]$ | |
| $p_{lmni}$ | item $i$ is placed in L3 virtual bin $(l, m, n)$ | bool | $\{0, 1\}$ | |

### Convention and Function

- define function $\textrm{w}(i) = w_{i} \cdot (1 - d_{i}) + h_{i} \cdot d_{i}$ to indicate the actual width of item $i$ regarding its rotation.
- define function $\textrm{h}(i) = h_{i} \cdot (1 - d_{i}) + w_{i} \cdot d_{i}$ to indicate the actual height of item $i$ regarding its rotation.


## Objective

### minimize the wasted raw material OWG (wasted glass)

reduce the geometrical loss of the cutting patterns.
only the glass at right of the last 1-cut performed in the last cutting pattern can be reused (regard as residual instead of waste).

$$
\min \sum_{l \in L^{1}} \omega_{l}
$$


## Constraint

there are several notations about the constraints.

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
  - user cuts or constraints that are very likely to be ignored when changing requirement or implementation
- lazy constraints
  - suffixed with **.L** in the tag
  - constraints which are recommended to be added lazily

all of the following constraints must be satisfied.

- **HHF.L (horizontal fitting)** the width of the item $i$ should be the same as the width of a L3 virtual bin $(l, m, n)$ if $i$ is placed in.
$W$ can be any value that is no less than $\omega^{3}_{lmn}$.
$$
\textrm{w}(i) \ge \omega^{3}_{lmn} - W \cdot (1 - p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
- **HHB (horizontal bound)** the width of the item $i$ should not exceed the width of a L3 virtual bin $(l, m, n)$ if $i$ is placed in.
$$
\textrm{w}(i) \le \omega^{3}_{lmn} + \max\{w_{i}, h_{i}\} \cdot (1 - p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$
- **HVB (vertical bound)** the height of the item $i$ should not exceed the height of a L2 virtual bin $(l, m)$ if $i$ is placed in.
$$
\textrm{h}(i) \le \eta^{2}_{lm} + \max\{w_{i}, h_{i}\} \cdot ( 1 - \sum_{n \in L^{3}_{lm}} p_{lmni}), \quad \forall i \in I, \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$

- **HEP (exclusive placement)** there should be no more than 1 item placed in single L3 virtual bin $(l, m, n)$.
$$
\sum_{i \in I} p_{lmni} \le 1, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HSP (single placement)** each item should be placed in exactly 1 L3 virtual bin.
$$
\sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} p_{lmni} = 1, \quad \forall i \in I
$$

- **HMW1 (L1 max width)** the sum of all L1 virtual bin's width should not exceed the width of the raw material.
$$
\sum_{l \in L^{1}} \omega^{1}_{l} \le W
$$
- **HMH2 (L2 max height)** the sum of all L2 virtual bin's height should not exceed the height of the raw material.
$$
\sum_{m \in L^{2}_{l}} \eta^{2}_{lm} \le H, \quad \forall l \in L^{1}
$$
- **HMW3 (L3 max width)** the sum of all L3 virtual bin's width should not exceed the width of its covering L1 virtual bin.
$$
\sum_{n \in L^{3}_{l}} \omega^{3}_{lmn} \le \omega^{1}_{l}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}
$$

- **HHO1.O (L1 horizontal order)** the wider virtual bins comes first (this may cut the optima if there are defects).
$$
\omega^{1}_{l} \ge \omega^{1}_{l'}, \quad \forall l, l' \in L^{1}, l' = l + 1
$$

- **HVO2.O (L2 vertical order)** the higher virtual bins comes first (this may cut the optima if there are defects).
$$
\omega^{2}_{lm} \ge \omega^{2}_{l'm'}, \quad \forall l \in L^{1}, \forall m, m' \in L^{2}_{l}, m' = m + 1
$$

- **HHO3.O (L3 horizontal order)** the wider virtual bins comes first (this may cut the optima if there are defects).
$$
\omega^{3}_{lmn} \ge \omega^{3}_{l'm'n'}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n, n' \in L^{3}_{lm}, n' = n + 1
$$
