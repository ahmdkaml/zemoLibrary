package com.zemolibrary.backend.controller;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

@RestController
public class HelloController {

    @GetMapping("/")
    public Map<String, String> root() {
        return Map.of(
                "status", "UP",
                "service", "ZemoServer",
                "version", "1.0.0"
        );
    }

    @GetMapping("/hello")
    public String hello() {
        return "hello";
    }
}
