#version 430 core

out vec4 fragColor;

uniform int selectedObjID;
uniform int objID;

void main(){
    fragColor = vec4(0,0,0,1);
    if(selectedObjID == objID){
        fragColor = vec4(1,1,1,1);
    }
}