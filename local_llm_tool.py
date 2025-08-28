#!/usr/bin/env python3
"""
Local LLM Tool for Claude Code
Provides access to locally running Ollama models via HTTP API
"""

import requests
import json
import sys
import argparse
from typing import Optional, Dict, Any

class LocalLLM:
    def __init__(self, base_url: str = "http://localhost:11434"):
        self.base_url = base_url
        self.api_url = f"{base_url}/api"
    
    def list_models(self) -> list:
        """List all available models"""
        try:
            response = requests.get(f"{self.api_url}/tags")
            response.raise_for_status()
            return response.json().get("models", [])
        except requests.RequestException as e:
            print(f"Error listing models: {e}")
            return []
    
    def generate(self, 
                model: str, 
                prompt: str, 
                system: Optional[str] = None,
                temperature: float = 0.1,
                max_tokens: Optional[int] = None) -> str:
        """Generate response from local LLM"""
        
        payload = {
            "model": model,
            "prompt": prompt,
            "stream": False,
            "options": {
                "temperature": temperature,
            }
        }
        
        if system:
            payload["system"] = system
        
        if max_tokens:
            payload["options"]["num_predict"] = max_tokens
        
        try:
            response = requests.post(f"{self.api_url}/generate", json=payload)
            response.raise_for_status()
            
            result = response.json()
            return result.get("response", "")
            
        except requests.RequestException as e:
            return f"Error: {e}"
        except json.JSONDecodeError as e:
            return f"JSON Error: {e}"

def main():
    parser = argparse.ArgumentParser(description='Local LLM Tool')
    parser.add_argument('--model', default='qwen2.5-coder:7b', help='Model to use')
    parser.add_argument('--list-models', action='store_true', help='List available models')
    parser.add_argument('--prompt', help='Prompt to send to model')
    parser.add_argument('--system', help='System message')
    parser.add_argument('--temperature', type=float, default=0.1, help='Temperature (0-1)')
    parser.add_argument('--max-tokens', type=int, help='Maximum tokens to generate')
    
    args = parser.parse_args()
    
    llm = LocalLLM()
    
    if args.list_models:
        models = llm.list_models()
        print("Available models:")
        for model in models:
            print(f"  - {model['name']}")
        return
    
    if not args.prompt:
        # Interactive mode - read from stdin
        prompt = sys.stdin.read().strip()
        if not prompt:
            print("Error: No prompt provided")
            sys.exit(1)
    else:
        prompt = args.prompt
    
    response = llm.generate(
        model=args.model,
        prompt=prompt,
        system=args.system,
        temperature=args.temperature,
        max_tokens=args.max_tokens
    )
    
    print(response)

if __name__ == "__main__":
    main()