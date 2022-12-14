# Wood

### Commit Rules

* NEW "" - new feature
* FIX "" - bug fix
* REF "" - refactoring, file removal, style
* DOC "" - documentation
* TES "" - tests

### Branches -> Create
* git branch <name>
* git checkout <name>

### Short-term To-do
  
- [ ] create a parametric joints whose number of tenons and chamfers could be controlled, this joint must be adapted to the length
- [ ] compute two additional connection volumes, meaning creating two additional joint with index(id0, id_tenon) and (id1, id_tenon)
- [ ] assign joint type
- [ ] orient it to the connection volumes(this is probably done in joint computatio step)
- [ ] remove male geometry and merge it with the id_tenon,


### Daily Screenshots

2022/10/30 - mooc chapel exercise, initial set using side-to-side connection
![viewer_30-10-2022_19-17-53](https://user-images.githubusercontent.com/18013985/198894740-3e976fe4-946b-4eb7-83e0-9d7826e69ebf.png)

2022/11/04 (midnight) - new joint "ss_e_op_4"
![image](https://user-images.githubusercontent.com/18013985/199853549-06f6720f-340c-405b-b12a-a7c6fb71e30b.png)

2022/11/04 (day) - adjustements to "ss_e_op_4" and extension parameter addition "wood_globals::joint_line_extension", needed to move joints away from corners
![image](https://user-images.githubusercontent.com/18013985/199975589-fee87e17-c8d5-463c-9be7-03b384371b78.png)

2022/11/13 - 4 valence joint implementation "ss_e_op_5"
![image](https://user-images.githubusercontent.com/18013985/201543201-0e0933ef-c85a-4f21-a460-44272f0bd7ca.png)

2022/11/19 - 4 valence joint implementation example "ss_e_op_5" and very similary customized for no tenons (property "disable_joint_divisions_1") "ss_e_op_6" 
![p1](https://user-images.githubusercontent.com/18013985/202870110-384aa41c-c365-4a7c-ac34-c574922a5abd.jpg)

2022/11/20 - examples addition: chapel_corner | chapel_one_layer | chapel_simple | chapel | todo: check why simple finger joints are not merged porperly with the outlines | implement 3 valence joint (when only one joint is connected not two)
![Chapel1](https://user-images.githubusercontent.com/18013985/202915055-02107a24-9fb5-4e85-86b8-3ec134f15229.jpg)


https://user-images.githubusercontent.com/18013985/202915294-fc893395-eecc-406a-8597-639f438726c2.mp4


