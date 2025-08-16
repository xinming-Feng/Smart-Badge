package com.example.nfcreader;

/**
 * GitHub configuration for image upload
 * 
 * SECURITY WARNING: Do not commit your actual token to version control!
 * This class should be customized for each user's GitHub setup.
 */
public class GitHubConfig {
    
    // TODO: Replace with your GitHub username
    public static final String GITHUB_USERNAME = "YOUR_USERNAME_HERE";
    
    // TODO: Replace with your repository name for image storage
    public static final String GITHUB_REPO = "YOUR_REPO_HERE";
    
    // TODO: Replace with your GitHub Personal Access Token
    // Generate at: https://github.com/settings/personal-access-tokens
    // Required permissions: repo
    public static final String GITHUB_TOKEN = "YOUR_TOKEN_HERE";
    
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