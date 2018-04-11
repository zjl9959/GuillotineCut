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
| $F$ | (**f**law) defect set | $[1, 200]$  | $f$ |               |
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
| $w_{i}, w_{f}$ | the width of the item $i$ or flaw $f$ | real | $(0, W]$    |        |
| $h_{i}, h_{f}$ | the height of the item $i$ or flaw $f$ | real | $(0, H]$    |        |
| $x_{f}$     | horizontal position of the flaw $f$'s left bottom                        | real | $[0, W]$ | in Cartesian coordinate system |
| $y_{f}$ | vertical position of the flaw $f$'s left bottom | real | $[0, H]$ | in Cartesian coordinate system |


## Decision

| Variable     | Description                                                | type | Domain     | Remark                                                     |
| -------------- | ------------------------------------------------------------ | ---- | ------------- | ------------------------------------------------------------ |
| $d_{i}$       | item $i$ is rotated 90 degree            | bool  | $\{0, 1\}$ |                                              |
| $\omega^{1}_{l}$ | width of the layer-1 virtual bin $l$ | real | $[0, W]$ | |
| $\eta^{2}_{lm}$ | height of the layer-2 virtual bin $m$ in L1 virtual bin $l$ | real | $[0, H]$ | |
| $\omega^{3}_{lmn}$ | width of the layer-3 virtual bin $n$ in L2 virtual bin $(l, m)$ | real | $[0, W]$ | |
| $p_{lmni}$ | item $i$ is placed in L3 virtual bin $(l, m, n)$ | bool | $\{0, 1\}$ | |
| $c_{lmnf}$ | L3 virtual bin $(l, m, n)$ contains flaw $f$ | bool | $\{0, 1\}$ | |
| $c^{k}_{lmnf}$ | L3 virtual bin $(l, m, n)$ is not on the $k^\textrm{th}$ side of flaw $f$ | bool | $\{0, 1\}$ | $k = \{1\textrm{:right}, 2\textrm{:left}, 3\textrm{:up}, 4\textrm{:down}\}$ |

### Convention and Function

- define function $\textrm{w}(i) = w_{i} \cdot (1 - d_{i}) + h_{i} \cdot d_{i}$ to indicate the actual width of item $i$ regarding its rotation.
- define function $\textrm{h}(i) = h_{i} \cdot (1 - d_{i}) + w_{i} \cdot d_{i}$ to indicate the actual height of item $i$ regarding its rotation.
- define function $\textrm{x}(l, m, n) = \sum\limits_{l' \in L^{1}, l' < l} \omega^{1}_{l'} + \sum\limits_{n' \in L^{3}_{lm}, n' < n} \omega^{3}_{lmn'}$ to indicate the horizontal position of L3 virtual bin $(l, m, n)$'s left bottom.
- define function $\textrm{y}(l, m) = \sum\limits_{m' \in L^{2}_{l}, m' < m} \omega^{2}_{lm'}$ to indicate the vertical position of L3 virtual bin $(l, m, n)$'s left bottom.


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

- **HSP (single placement)** each item should be placed in exactly 1 L3 virtual bin.
$$
\sum_{l \in L^{1}} \sum_{m \in L^{2}_{l}} \sum_{n \in L^{3}_{lm}} p_{lmni} = 1, \quad \forall i \in I
$$
- **HEP (exclusive placement)** there should be no more than 1 item placed in single L3 virtual bin $(l, m, n)$, and it will be a trivial constraint if **HDF (defect free)** is enabled.
$$
\sum_{i \in I} p_{lmni} \le 1, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}
$$

- **HDF (defect free)** there should be no item placed in L3 virtual bin $(l, m, n)$ if it contains defects, and enabling it will make **HEP (exclusive placement)** a trivial constraint.
$$
\sum_{i \in I} p_{lmni} \le 1 - c_{lmnf}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
- **HDC (defect containing)** L3 virtual bin $(l, m, n)$ contains flaw $f$ if it is not on any side of flaw $f$.
$$
c_{lmnf} \ge \bigwedge^{4}_{k = 1} c^{k}_{lmnf}
\ \Leftrightarrow\ 
3 + c_{lmnf} \ge \sum^{4}_{k = 1} c^{k}_{lmnf}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
- **HDD (defect direction)** L3 virtual bin $(l, m, n)$ is on some sides of flaw $f$.
$$
\textrm{x}(l, m, n) + W \cdot c^{1}_{lmnf} \ge x_{f} + w_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{x}(l, m, n) + \omega^{3}_{lmn} + W \cdot c^{2}_{lmnf} \le x_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{y}(l, m) + H \cdot c^{3}_{lmnf} \ge y_{f} + h_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
$$
$$
\textrm{y}(l, m) + \omega^{2}_{lm} + H \cdot c^{4}_{lmnf} \le y_{f}, \quad \forall l \in L^{1}, \forall m \in L^{2}_{l}, \forall n \in L^{3}_{lm}, \forall f \in F
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
