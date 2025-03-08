# HextrixAI - Advanced AI System with OS Integration

HextrixAI is a cutting-edge artificial intelligence system that combines a powerful multimodal AI assistant with low-level OS integration capabilities. This project aims to create a comprehensive AI ecosystem that can interact with users through multiple modalities while leveraging deep system integration.

## Core Features

### Multimodal AI Capabilities
- **Text Processing**: Advanced language understanding using multiple LLM backends (Llama 3.3, Gemini 2.0, Gemma)
- **Image Processing**: Computer vision with LLaVA, image generation with Stable Diffusion XL and Flux
- **Speech Recognition**: Audio processing and transcription with Whisper
- **Emotional Intelligence**: Sentiment analysis and emotional context awareness
- **Real-time Processing**: Support for video streams and multi-modal interactions

### System Integration
- **Custom OS Kernel**: Low-level integration with a specialized x86_64 kernel
- **Memory Management**: Advanced memory allocation and paging system
- **Interrupt Handling**: Hardware interaction through sophisticated interrupt management
- **Terminal Interface**: Direct hardware access for system-level control

### API Integrations
- **Google Services**: Integration with Gmail, Photos, Drive, Fitness, and other Google APIs
- **External Search**: Perplexity API for research and Google SERP API for web search
- **Cloud Storage**: Neural memory with cloud synchronization

### User Experience
- **Emotion-Aware Responses**: System adapts to user sentiment and maintains its emotional context
- **Persistent Memory**: Long-term conversation tracking and sentiment analysis
- **Self-Awareness System**: AI can reflect on its own capabilities and suggest improvements

## Getting Started

### Prerequisites
- 64-bit x86 system for OS components
- Python 3.8+ for AI components
- GPU with CUDA support (recommended)
- API keys for various services:
  - Google API
  - Cloudflare API
  - Perplexity API
  - SERP API

### Installation

For the AI System:
```bash
# Clone the repository
git clone https://github.com/yourusername/hextrixai.git

# Navigate to the project directory
cd hextrixai

# Install dependencies
pip install -r requirements.txt

# Set up environment variables
cp .env.example .env
# Edit .env with your API keys

# Run the application
python app.py
```

For the OS Components:
```bash
# Build the kernel
make

# Create an ISO image
make iso

# Test in a virtual machine
# (Use QEMU, VirtualBox, or similar)
```

## Project Structure

- `app.py` - Main AI system entry point
- `src/` - OS kernel source files
- `include/` - OS header files
- `mem_drive.py` - Neural memory management
- `self_awareness.py` - Self-reflection system
- `google_api_manager.py` - Google service integrations

## Future Roadmap

See [TODO.md](TODO.md) for our comprehensive development roadmap.

## Change History

See [CHANGELOG.md](CHANGELOG.md) for the version history and list of changes.

## License

This project is licensed under the MIT License - see [LICENSE.md](LICENSE.md) for details.

## Acknowledgments

- Various open-source AI models used in this project
- Contributors and researchers in AI and OS development
