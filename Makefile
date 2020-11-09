# Makefile

# Final files
OBJECTS = pd user as fs

# Compiler
CPP = g++

# Command used at clean
RM = rm -rf

all: $(OBJECTS)
	
#$(OBJECTS): %: %.cpp
#	@echo "\033[1;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m" $<
#	@$(CPP) -o $@ $< 

pd: PD.cpp
	@echo "\033[1;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m PD.cpp"
	@$(CPP) -o pd PD.cpp

user: User.cpp
	@echo "\033[1;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m User.cpp"
	@$(CPP) -o user User.cpp 

as: AS.cpp
	@echo "\033[1;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m AS.cpp"
	@$(CPP) -o as AS.cpp

fs: FS.cpp
	@echo "\033[1;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m FS.cpp"
	@$(CPP) -o fs FS.cpp

clean:
	@if test -s pd; \
    then echo "\033[1;32m [OK] \033[0m       \033[0;33m Removing Files:\033[0m $(OBJECTS)"; \
	$(RM) $(OBJECTS); \
    else echo "\033[0;31m [ERROR] \033[0m"; \
    fi
