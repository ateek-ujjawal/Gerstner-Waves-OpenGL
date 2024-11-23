#include "Camera.hpp"

#include "glm/gtx/transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include <iostream>

void Camera::MouseLook(int mouseX, int mouseY){
    // Record our new position as a vector
    glm::vec2 newMousePosition(mouseX, mouseY);
    // Little hack for our 'mouse look function'
    // We need this so that we can move our camera
    // for the first time.
    static bool firstLook=true;
    if(true == firstLook){
        firstLook=false;
        m_oldMousePosition = newMousePosition;
    }

    // Detect how much the mouse has moved since
    // the last time
    // Calculate delta
    glm::vec2 delta = m_oldMousePosition - newMousePosition;

    // Convert delta to a radian angle
    // Then rotate about the up vector
    glm::mat4 rotationModelX = glm::rotate(glm::mat4(1.0f), glm::radians(delta.x), m_upVector);
    glm::mat4 rotationModelY = glm::rotate(rotationModelX, glm::radians(delta.y), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec4 vec4View = glm::vec4(m_viewDirection.x, m_viewDirection.y, m_viewDirection.z, 1.0f);
    vec4View = rotationModelY * vec4View;
    m_viewDirection = glm::vec3(vec4View.x, vec4View.y, vec4View.z);

    // Update our old position after we have made changes 
    m_oldMousePosition = newMousePosition;
}

void Camera::MoveForward(float speed){
    //   TODO -- consider which glm::vec3 you are updating, and by what factor
    // Move forward in the direction of the view
    m_eyePosition += m_viewDirection * speed;
}

void Camera::MoveBackward(float speed){
    //   TODO -- consider which glm::vec3 you are updating, and by what factor
    // Move backward in the direction of the view
    m_eyePosition -= m_viewDirection * speed;
}

void Camera::MoveLeft(float speed){
    //   TODO -- Note: You are updating the 'eye' position, but consider
    //        	       that you will need to update the 'rightVector'
    // Compute the right vector and update your 'eye' accordingly
    // Move left in the direction of the computed cross product of direction and up vector
    m_eyePosition -= glm::normalize(glm::cross(m_viewDirection, m_upVector)) * speed;
}

void Camera::MoveRight(float speed){
    //   TODO -- Note: You are updating the 'eye' position, but consider
    //        	       that you will need to update the 'rightVector'
    // Compute the right vector and update your 'eye' accordingly
    // Move right in the direction of the computed cross product of direction and up vector
    m_eyePosition += glm::normalize(glm::cross(m_viewDirection, m_upVector)) * speed;
}

void Camera::MoveUp(float speed){
    m_eyePosition.y += speed;
}

void Camera::MoveDown(float speed){
    m_eyePosition.y -= speed;
}

// Set the position for the camera
void Camera::SetCameraEyePosition(float x, float y, float z){
    m_eyePosition.x = x;
    m_eyePosition.y = y;
    m_eyePosition.z = z;
}

float Camera::GetEyeXPosition(){
    return m_eyePosition.x;
}

float Camera::GetEyeYPosition(){
    return m_eyePosition.y;
}

float Camera::GetEyeZPosition(){
    return m_eyePosition.z;
}

float Camera::GetViewXDirection(){
    return m_viewDirection.x;
}

float Camera::GetViewYDirection(){
    return m_viewDirection.y;
}

float Camera::GetViewZDirection(){
    return m_viewDirection.z;
}


Camera::Camera(){
    std::cout << "Camera.cpp: (Constructor) Created a Camera!\n";
	// Position us at the origin.
    m_eyePosition = glm::vec3(0.0f,0.0f, 3.0f);
	// Looking down along the z-axis initially.
	// Remember, this is negative because we are looking 'into' the scene.
    m_viewDirection = glm::vec3(0.0f,0.0f, -1.0f);
	// For now--our upVector always points up along the y-axis
    m_upVector = glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 Camera::GetViewMatrix() const{
    // Think about the second argument and why that is
    // setup as it is.
    return glm::lookAt( m_eyePosition,
                        m_eyePosition + m_viewDirection,
                        m_upVector);
}
