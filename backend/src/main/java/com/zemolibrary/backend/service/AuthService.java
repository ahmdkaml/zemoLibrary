package com.zemolibrary.backend.service;

import com.zemolibrary.backend.dto.AuthDTOs.*;
import com.zemolibrary.backend.model.User;
import com.zemolibrary.backend.repository.UserRepository;
import com.zemolibrary.backend.util.PasswordUtil;
import org.springframework.stereotype.Service;

import java.util.Optional;

@Service
public class AuthService {

    private final UserRepository userRepository;

    public AuthService(UserRepository userRepository) {
        this.userRepository = userRepository;
    }

    public AuthResponse signup(SignupRequest request) {
        if (request.getEmail() == null || request.getEmail().trim().isEmpty()) {
            return AuthResponse.failure("Email is required");
        }
        if (request.getPassword() == null || request.getPassword().length() < 6) {
            return AuthResponse.failure("Password must be at least 6 characters");
        }

        String normalizedEmail = request.getEmail().trim().toLowerCase();
        if (userRepository.existsByEmail(normalizedEmail)) {
            return AuthResponse.failure("Email is already registered");
        }

        String salt = PasswordUtil.generateSalt();
        String passwordHash = PasswordUtil.hashPassword(request.getPassword(), salt);

        User user = new User(
                request.getName() != null ? request.getName().trim() : null,
                normalizedEmail,
                passwordHash,
                salt
        );

        User saved = userRepository.save(user);
        UserDTO userDto = new UserDTO(saved.getId(), saved.getName(), saved.getEmail());
        return AuthResponse.success("User registered successfully", userDto);
    }

    public AuthResponse login(LoginRequest request) {
        if (request.getEmail() == null || request.getEmail().trim().isEmpty()) {
            return AuthResponse.failure("Email is required");
        }
        if (request.getPassword() == null || request.getPassword().isEmpty()) {
            return AuthResponse.failure("Password is required");
        }

        String normalizedEmail = request.getEmail().trim().toLowerCase();
        Optional<User> optionalUser = userRepository.findByEmail(normalizedEmail);
        if (optionalUser.isEmpty()) {
            return AuthResponse.failure("Invalid email or password");
        }

        User user = optionalUser.get();
        boolean valid = PasswordUtil.verifyPassword(request.getPassword(), user.getSalt(), user.getPasswordHash());
        if (!valid) {
            return AuthResponse.failure("Invalid email or password");
        }

        UserDTO userDto = new UserDTO(user.getId(), user.getName(), user.getEmail());
        return AuthResponse.success("Login successful", userDto);
    }
}
