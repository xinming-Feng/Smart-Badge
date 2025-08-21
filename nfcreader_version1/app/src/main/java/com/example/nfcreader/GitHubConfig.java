package com.example.nfcreader;

/**
 * GitHub configuration for image upload
 * 
 * This class contains the GitHub repository and token configuration
 * for uploading processed images to GitHub for NFC transmission.
 */
public class GitHubConfig {
    
    // GitHub username and repository for image storage
    public static final String GITHUB_USERNAME = "xinming-Feng";
    public static final String GITHUB_REPO = "image";
    
    // GitHub Personal Access Token for API access
    // This token has repo permissions for uploading files
    public static final String GITHUB_TOKEN = "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";
    
    /**
     * Get GitHub API URL for file upload
     */
    public static String getApiUrl(String path) {
        return "https://api.github.com/repos/" + GITHUB_USERNAME + "/" + GITHUB_REPO + "/contents/" + path;
    }
    
    /**
     * Get GitHub Raw URL for file access
     */
    public static String getRawUrl(String path) {
        return "https://raw.githubusercontent.com/" + GITHUB_USERNAME + "/" + GITHUB_REPO + "/main/" + path;
    }
    
    /**
     * Get authorization header value
     */
    public static String getAuthHeader() {
        return "token " + GITHUB_TOKEN;
    }
}
