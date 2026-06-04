# Contributing

Firstly thanks for contributing, here is the main dev workflow of the repo. Make sure to consult all of the documentation within this folder, there is a breif overview of what each canframe does and what each of the functionality on the motors looks like. The PDF inside this folder is `CubeMars` provided for the motor controller boards, this is the protocol we are writing code for, make sure you consult against this code for that information

## Workflow

The main workflow of the repo is working through a dev container using can utils. In the repo route there is a docker file which specifically provides the can utils we need. Additionally we are using `cmake` as our main build system
To initialize the cmake folder, here are the commands to setup

```bash
mkdir build
cd build
cmake -S . -B build -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON # optionally, if you use a IDE that needs a compile commands file
```

From here if you have the proper Cmake plugin/LSP config and dev container setup, you should be good to go. Reach out if you are having issues and are a member of trickfire.

## Branch naming

Make sure to follow standard git standards
| Branch heading| Defintion |
| ------------- | -------------- |
| /feat/<name> | You are introducing a new feature to the codebase |
| /fix/<name> | You are contributing a fix to a known issue |
| /docs/<name> | You are contributing a fix to the documentation/contributing some documentation |

## Commit naming

| Message     | Description                                |
| ----------- | ------------------------------------------ |
| `feat:`     | Including a new feature                    |
| `fix:`      | Bug fix                                    |
| `chore:`    | Maintenace/config changes, no logic change |
| `docs:`     | A change to the documentation              |
| `style:`    | Formatting/style changes, no logic changes |
| `refactor:` | Refactoring/restructuring                  |
| `perf:`     | Performance changes                        |
| `ci:`       | CI changes                                 |
| `revert:`   | Reverting a change                         |

### `Examples`

```
feat: added new bindings for motor velocity
fix: fixed a memory corruption bug in `Servo frame`
docs: added the motor board pdf into the docs folder
ci: fixed formatter workflow
```

## Code style

For function declarations, don't declare functions with the trailing return type syntax, follow the return type first syntax unless you are creating a template function that requires it. Follow standard camel case function naming conventions for functions and variables.

```C++
//Do this
void doSomething() { ... }
//NOT THIS
auto doSomethingElse() -> void { ... }
//This is ok
template <typename T, typename B>
auto doSomethingTemplate(T a, B b) -> decltype(T, B) { ... }
```

For enum's and classes make sure you capitalize the name of every single word in the enum class.

```C++
enum class MitHeaderEnum {
    FrameDoesSomething = 0,
    FrameDoesSomethingElse
};

class MitFrame {
...
};
```

Other than this, most of the formatting gets handled by the formatter on push to main. The the code will be formatted there.

## Testing

For our testing suite, we will use GTest's which will also be a part of the CD pipeline, when you make a PR to main, make sure that your code passes all tests, and if you include new functionality you are required to include it in your own tests for that new functionality in the appropriate folders.
