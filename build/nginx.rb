#  Phusion Passenger - https://www.phusionpassenger.com/
#  Copyright (c) 2010-2016 Phusion Holding B.V.
#
#  "Passenger", "Phusion Passenger" and "Union Station" are registered
#  trademarks of Phusion Holding B.V.
#
#  See LICENSE file for license information.

auto_generated_sources = %w(
  src/nginx_module/ConfigurationCommands.c
  src/nginx_module/CreateLocationConfig.c
  src/nginx_module/MergeLocationConfig.c
  src/nginx_module/CacheLocationConfig.c
  src/nginx_module/LocationConfig.h
)

desc "Build Nginx support files"
task :nginx => [
  :nginx_without_native_support,
  NATIVE_SUPPORT_TARGET
].compact

desc "Build Nginx support files, including objects suitable for dynamic linking against Nginx"
task 'nginx:as_dynamic_module' => [
  :nginx_dynamic_without_native_support,
  NATIVE_SUPPORT_TARGET
].compact

# Workaround for https://github.com/jimweirich/rake/issues/274
task :_nginx => :nginx

task :nginx_without_native_support => [
  auto_generated_sources,
  AGENT_TARGET,
  COMMON_LIBRARY.only(*NGINX_LIBS_SELECTOR).link_objects
].flatten

# define_tasks creates an extra compilation to the specified output_dir, with extra compiler flags;
# it also creates a namespace:clean task to clean up the output_dir
task :nginx_dynamic_without_native_support => [
  auto_generated_sources,
  AGENT_TARGET,
  define_libboost_oxt_task("nginx", NGINX_DYNAMIC_OUTPUT_DIR + "libboost_oxt", "-fPIC"),
  COMMON_LIBRARY.only(*NGINX_LIBS_SELECTOR).
    set_namespace("nginx").set_output_dir(NGINX_DYNAMIC_OUTPUT_DIR + "module_libpassenger_common").define_tasks("-fPIC").
    link_objects
].flatten

task :clean => 'nginx:clean'
desc "Clean all compiled Nginx files"
task 'nginx:clean' => 'common:clean' do
  # Nothing extra to clean at this time.
end

def create_nginx_auto_generated_source_task(source)
  dependencies = [
    "#{source}.cxxcodebuilder",
    'src/ruby_supportlib/phusion_passenger/nginx/config_options.rb'
  ]
  file(source => dependencies) do
    template = CxxCodeTemplateRenderer.new("#{source}.cxxcodebuilder")
    template.render_to(source)
  end
end

auto_generated_sources.each do |source|
  create_nginx_auto_generated_source_task(source)
end
